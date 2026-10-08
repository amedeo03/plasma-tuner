// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QAudioDevice>
#include <QAudioFormat>
#include <QObject>

#include <memory>
#include <vector>

class QAudioSource;
class QIODevice;
class PitchDetector;

// Captures audio from an input device and runs pitch detection on it.
// Lives in a worker thread: only call its slots through queued invocations.
class AudioCapture : public QObject
{
    Q_OBJECT

public:
    explicit AudioCapture(QObject *parent = nullptr);
    ~AudioCapture() override;

    void start(const QAudioDevice &device);
    void stop();

Q_SIGNALS:
    // frequency is 0 when no pitch was detected; level is the RMS in dBFS.
    void analysed(double frequency, double clarity, double level);
    void errorOccurred(const QString &message);

private:
    void readAvailable();
    void analyse();

    std::unique_ptr<QAudioSource> m_source;
    QIODevice *m_io = nullptr;
    QAudioFormat m_format;
    std::unique_ptr<PitchDetector> m_detector;
    QByteArray m_pending; // bytes of an incomplete frame
    std::vector<std::vector<float>> m_channels; // recent samples, one buffer per channel
    std::vector<float> m_window;
    std::size_t m_hopSize = 0;
    std::size_t m_sinceAnalysis = 0;
};
