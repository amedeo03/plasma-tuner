// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "audiocapture.h"

#include "core/pitchdetector.h"

#include <KLocalizedString>

#include <QAudioSource>

#include <bit>
#include <cmath>
#include <numeric>

namespace
{
// Analysis window length; ~160 ms keeps two periods of the lowest bass notes.
constexpr double windowSeconds = 0.16;
constexpr double analysesPerSecond = 25.0;
// Below this RMS level the input is considered silent.
constexpr double noiseGateDb = -60.0;
}

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
{
}

AudioCapture::~AudioCapture()
{
    stop();
}

void AudioCapture::start(const QAudioDevice &device)
{
    stop();
    if (device.isNull()) {
        Q_EMIT errorOccurred(i18nc("@info", "No audio input device available."));
        return;
    }

    m_format = device.preferredFormat();
    QAudioFormat floatFormat = m_format;
    floatFormat.setSampleFormat(QAudioFormat::Float);
    if (device.isFormatSupported(floatFormat)) {
        m_format = floatFormat;
    }
    if (!m_format.isValid() || m_format.channelCount() < 1) {
        Q_EMIT errorOccurred(i18nc("@info", "Unsupported audio format on %1.", device.description()));
        return;
    }

    const auto sampleRate = static_cast<double>(m_format.sampleRate());
    const std::size_t windowSize = std::bit_ceil(static_cast<std::size_t>(sampleRate * windowSeconds));
    m_detector = std::make_unique<PitchDetector>(sampleRate, windowSize);
    m_hopSize = static_cast<std::size_t>(sampleRate / analysesPerSecond);
    m_sinceAnalysis = 0;
    m_window.assign(windowSize, 0.0f);
    m_channels.assign(m_format.channelCount(), {});
    m_pending.clear();

    m_source = std::make_unique<QAudioSource>(device, m_format);
    m_source->setBufferSize(m_format.bytesForDuration(50000));
    connect(m_source.get(), &QAudioSource::stateChanged, this, [this] {
        if (m_source && m_source->error() != QAudio::NoError) {
            Q_EMIT errorOccurred(i18nc("@info", "Could not record from the selected input."));
            // Don't destroy the source from within its own signal.
            QMetaObject::invokeMethod(this, &AudioCapture::stop, Qt::QueuedConnection);
        }
    });
    m_io = m_source->start();
    if (!m_io) {
        Q_EMIT errorOccurred(i18nc("@info", "Could not record from the selected input."));
        stop();
        return;
    }
    connect(m_io, &QIODevice::readyRead, this, &AudioCapture::readAvailable);
}

void AudioCapture::stop()
{
    if (m_source) {
        m_source->disconnect(this);
        m_source->stop();
        m_source.reset();
    }
    m_io = nullptr;
    m_detector.reset();
    m_channels.clear();
}

void AudioCapture::readAvailable()
{
    if (!m_io) {
        return;
    }
    m_pending += m_io->readAll();

    const int frameBytes = m_format.bytesPerFrame();
    const int sampleBytes = m_format.bytesPerSample();
    const int channelCount = m_format.channelCount();
    const qsizetype frames = m_pending.size() / frameBytes;
    const char *data = m_pending.constData();
    const std::size_t windowSize = m_window.size();

    for (qsizetype frame = 0; frame < frames; ++frame) {
        for (int channel = 0; channel < channelCount; ++channel) {
            m_channels[channel].push_back(m_format.normalizedSampleValue(data + frame * frameBytes + channel * sampleBytes));
        }
        if (++m_sinceAnalysis >= m_hopSize && m_channels.front().size() >= windowSize) {
            m_sinceAnalysis = 0;
            analyse();
        }
    }
    m_pending.remove(0, frames * frameBytes);

    // Keep only the most recent window of samples.
    for (auto &samples : m_channels) {
        if (samples.size() > windowSize) {
            samples.erase(samples.begin(), samples.end() - static_cast<std::ptrdiff_t>(windowSize));
        }
    }
}

void AudioCapture::analyse()
{
    const std::size_t windowSize = m_window.size();

    // Audio interfaces often carry the instrument on a single channel of a
    // multichannel source: analyse whichever channel is loudest.
    const std::vector<float> *loudest = nullptr;
    double loudestEnergy = -1.0;
    for (const auto &samples : m_channels) {
        const auto begin = samples.end() - static_cast<std::ptrdiff_t>(windowSize);
        const double energy = std::inner_product(begin, samples.end(), begin, 0.0);
        if (energy > loudestEnergy) {
            loudestEnergy = energy;
            loudest = &samples;
        }
    }

    // Copy the window, removing any DC offset.
    const auto begin = loudest->end() - static_cast<std::ptrdiff_t>(windowSize);
    const double mean = std::accumulate(begin, loudest->end(), 0.0) / static_cast<double>(windowSize);
    double energy = 0.0;
    for (std::size_t i = 0; i < windowSize; ++i) {
        m_window[i] = static_cast<float>(begin[static_cast<std::ptrdiff_t>(i)] - mean);
        energy += double(m_window[i]) * m_window[i];
    }
    const double rms = std::sqrt(energy / static_cast<double>(windowSize));
    const double level = rms > 0.0 ? 20.0 * std::log10(rms) : -120.0;

    if (level < noiseGateDb) {
        Q_EMIT analysed(0.0, 0.0, level);
        return;
    }
    const PitchDetector::Result result = m_detector->detect(m_window.data());
    Q_EMIT analysed(result.frequency, result.clarity, level);
}
