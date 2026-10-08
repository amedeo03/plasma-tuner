// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "tuner.h"

#include "audiocapture.h"
#include "core/notes.h"

#include <KLocalizedString>

#include <algorithm>
#include <cmath>

using namespace std::chrono_literals;

namespace
{
// Detections less periodic than this are treated as noise.
constexpr double minimumClarity = 0.8;
constexpr double minimumFrequency = 20.0;
constexpr double maximumFrequency = 1500.0;
// Median filter length over successive detections.
constexpr qsizetype smoothingLength = 5;
// Meter range: -60 dBFS maps to 0, 0 dBFS to 1.
constexpr double meterFloorDb = -60.0;
}

Tuner::Tuner(QObject *parent)
    : QObject(parent)
{
    m_capture = new AudioCapture;
    m_capture->moveToThread(&m_thread);
    m_thread.setObjectName(QStringLiteral("TunerAudio"));
    m_thread.start();

    connect(m_capture, &AudioCapture::analysed, this, &Tuner::handleAnalysis);
    connect(m_capture, &AudioCapture::errorOccurred, this, [this](const QString &message) {
        m_running = false;
        m_runningDeviceId.clear();
        setErrorString(message);
    });

    m_holdTimer.setSingleShot(true);
    m_holdTimer.setInterval(600ms);
    connect(&m_holdTimer, &QTimer::timeout, this, [this] {
        setHasSignal(false);
    });

    connect(&m_mediaDevices, &QMediaDevices::audioInputsChanged, this, [this] {
        refreshDevices();
        updateCapture();
    });
    refreshDevices();
}

Tuner::~Tuner()
{
    QMetaObject::invokeMethod(m_capture, &AudioCapture::stop, Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
    delete m_capture;
}

bool Tuner::isActive() const
{
    return m_active;
}

void Tuner::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    Q_EMIT activeChanged();
    updateCapture();
}

QString Tuner::deviceId() const
{
    return m_deviceId;
}

void Tuner::setDeviceId(const QString &deviceId)
{
    if (m_deviceId == deviceId) {
        return;
    }
    m_deviceId = deviceId;
    Q_EMIT deviceIdChanged();
    Q_EMIT deviceIndexChanged();
    updateCapture();
}

double Tuner::referencePitch() const
{
    return m_referencePitch;
}

void Tuner::setReferencePitch(double referencePitch)
{
    if (referencePitch <= 0.0 || qFuzzyCompare(m_referencePitch, referencePitch)) {
        return;
    }
    m_referencePitch = referencePitch;
    Q_EMIT referencePitchChanged();
    updateReading();
}

QVariantList Tuner::devices() const
{
    QVariantList list;
    const QAudioDevice defaultInput = QMediaDevices::defaultAudioInput();
    const QString defaultName = defaultInput.isNull() ? i18nc("@item:inlistbox", "System default")
                                                      : i18nc("@item:inlistbox %1 is a device name", "System default (%1)", defaultInput.description());
    list.append(QVariantMap{{QStringLiteral("id"), QString()}, {QStringLiteral("name"), defaultName}});
    for (const QAudioDevice &device : m_inputs) {
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromUtf8(device.id())}, {QStringLiteral("name"), device.description()}});
    }
    return list;
}

int Tuner::deviceIndex() const
{
    if (m_deviceId.isEmpty()) {
        return 0;
    }
    const QByteArray id = m_deviceId.toUtf8();
    const auto it = std::find_if(m_inputs.cbegin(), m_inputs.cend(), [&](const QAudioDevice &device) {
        return device.id() == id;
    });
    return it == m_inputs.cend() ? 0 : static_cast<int>(it - m_inputs.cbegin()) + 1;
}

QString Tuner::errorString() const
{
    return m_errorString;
}

double Tuner::level() const
{
    return m_level;
}

bool Tuner::hasSignal() const
{
    return m_hasSignal;
}

double Tuner::frequency() const
{
    return m_frequency;
}

int Tuner::midiNote() const
{
    return m_midiNote;
}

QString Tuner::noteName() const
{
    return m_midiNote < 0 ? QString() : Notes::name(m_midiNote);
}

int Tuner::octave() const
{
    return m_midiNote < 0 ? 0 : Notes::octave(m_midiNote);
}

double Tuner::cents() const
{
    return m_cents;
}

void Tuner::refreshDevices()
{
    m_inputs = QMediaDevices::audioInputs();
    Q_EMIT devicesChanged();
    Q_EMIT deviceIndexChanged();
}

QAudioDevice Tuner::resolveDevice() const
{
    // A saved device that is currently unplugged falls back to the default,
    // without forgetting the user's choice.
    const int index = deviceIndex();
    return index == 0 ? QMediaDevices::defaultAudioInput() : m_inputs.at(index - 1);
}

void Tuner::updateCapture()
{
    if (!m_active) {
        if (m_running) {
            QMetaObject::invokeMethod(m_capture, &AudioCapture::stop);
            m_running = false;
            m_runningDeviceId.clear();
        }
        m_holdTimer.stop();
        m_recentFrequencies.clear();
        setHasSignal(false);
        if (m_level != 0.0) {
            m_level = 0.0;
            Q_EMIT levelChanged();
        }
        return;
    }

    const QAudioDevice device = resolveDevice();
    if (m_running && device.id() == m_runningDeviceId) {
        return;
    }
    setErrorString(QString());
    m_running = true;
    m_runningDeviceId = device.id();
    m_recentFrequencies.clear();
    QMetaObject::invokeMethod(m_capture, [capture = m_capture, device] {
        capture->start(device);
    });
}

void Tuner::handleAnalysis(double frequency, double clarity, double level)
{
    if (!m_active) {
        return; // stale result from a capture being stopped
    }

    const double meter = std::clamp((level - meterFloorDb) / -meterFloorDb, 0.0, 1.0);
    if (!qFuzzyCompare(1.0 + meter, 1.0 + m_level)) {
        m_level = meter;
        Q_EMIT levelChanged();
    }

    if (frequency < minimumFrequency || frequency > maximumFrequency || clarity < minimumClarity) {
        return; // let the hold timer expire the current note
    }

    // A fresh note after silence must not be smoothed with the previous one.
    if (!m_hasSignal) {
        m_recentFrequencies.clear();
    }
    m_recentFrequencies.append(frequency);
    if (m_recentFrequencies.size() > smoothingLength) {
        m_recentFrequencies.removeFirst();
    }
    QList<double> sorted = m_recentFrequencies;
    std::sort(sorted.begin(), sorted.end());
    m_frequency = sorted.at(sorted.size() / 2);

    m_holdTimer.start();
    updateReading();
    setHasSignal(true);
}

void Tuner::updateReading()
{
    if (m_frequency <= 0.0) {
        return;
    }
    const double midi = Notes::midiFromFrequency(m_frequency, m_referencePitch);
    m_midiNote = static_cast<int>(std::lround(midi));
    m_cents = (midi - m_midiNote) * 100.0;
    Q_EMIT readingChanged();
}

void Tuner::setHasSignal(bool hasSignal)
{
    if (m_hasSignal == hasSignal) {
        return;
    }
    m_hasSignal = hasSignal;
    Q_EMIT hasSignalChanged();
}

void Tuner::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }
    m_errorString = errorString;
    Q_EMIT errorStringChanged();
}
