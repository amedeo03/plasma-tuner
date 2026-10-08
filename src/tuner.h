// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QAudioDevice>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <QQmlEngine>
#include <QThread>
#include <QTimer>
#include <QVariantList>

class AudioCapture;

// Chromatic tuner: listens on an input device while active and reports the
// nearest note and its deviation in cents.
class Tuner : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // Whether the microphone is open. Keep it false while the UI is hidden.
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    // Selected input device id; empty means the system default.
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    // Frequency of A4 in Hz.
    Q_PROPERTY(double referencePitch READ referencePitch WRITE setReferencePitch NOTIFY referencePitchChanged)

    // Input devices as {id, name} maps, the system default first (id "").
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    // Index in devices of the selected device, falling back to the default.
    Q_PROPERTY(int deviceIndex READ deviceIndex NOTIFY deviceIndexChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    // Input level, 0 (silence) to 1 (full scale).
    Q_PROPERTY(double level READ level NOTIFY levelChanged)
    // True while a pitch is being detected. The reading below keeps the last
    // detected note after the signal fades.
    Q_PROPERTY(bool hasSignal READ hasSignal NOTIFY hasSignalChanged)
    Q_PROPERTY(double frequency READ frequency NOTIFY readingChanged)
    Q_PROPERTY(int midiNote READ midiNote NOTIFY readingChanged)
    Q_PROPERTY(QString noteName READ noteName NOTIFY readingChanged)
    Q_PROPERTY(int octave READ octave NOTIFY readingChanged)
    // Deviation from the nearest note, -50 to +50.
    Q_PROPERTY(double cents READ cents NOTIFY readingChanged)

public:
    explicit Tuner(QObject *parent = nullptr);
    ~Tuner() override;

    bool isActive() const;
    void setActive(bool active);
    QString deviceId() const;
    void setDeviceId(const QString &deviceId);
    double referencePitch() const;
    void setReferencePitch(double referencePitch);

    QVariantList devices() const;
    int deviceIndex() const;
    QString errorString() const;

    double level() const;
    bool hasSignal() const;
    double frequency() const;
    int midiNote() const;
    QString noteName() const;
    int octave() const;
    double cents() const;

Q_SIGNALS:
    void activeChanged();
    void deviceIdChanged();
    void referencePitchChanged();
    void devicesChanged();
    void deviceIndexChanged();
    void errorStringChanged();
    void levelChanged();
    void hasSignalChanged();
    void readingChanged();

private:
    void refreshDevices();
    void updateCapture();
    QAudioDevice resolveDevice() const;
    void handleAnalysis(double frequency, double clarity, double level);
    void updateReading();
    void setHasSignal(bool hasSignal);
    void setErrorString(const QString &errorString);

    QMediaDevices m_mediaDevices;
    QList<QAudioDevice> m_inputs;
    QThread m_thread;
    AudioCapture *m_capture = nullptr;
    QByteArray m_runningDeviceId;
    bool m_running = false;

    bool m_active = false;
    QString m_deviceId;
    double m_referencePitch = 440.0;
    QString m_errorString;

    QTimer m_holdTimer;
    QList<double> m_recentFrequencies;
    double m_level = 0.0;
    bool m_hasSignal = false;
    double m_frequency = 0.0;
    int m_midiNote = -1;
    double m_cents = 0.0;
};
