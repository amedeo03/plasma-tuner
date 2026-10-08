// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "core/notes.h"
#include "core/pitchdetector.h"

#include <QTest>

#include <cmath>
#include <numbers>
#include <random>

namespace
{
constexpr std::size_t windowSize = 8192;

// Plucked-string-like tone: decaying harmonics, optionally with a weak fundamental.
std::vector<float> tone(double frequency, double sampleRate, double fundamentalGain = 1.0, double noise = 0.0)
{
    std::vector<float> samples(windowSize);
    std::mt19937 rng(42);
    std::normal_distribution<double> gaussian(0.0, noise > 0.0 ? noise : 1.0);
    for (std::size_t i = 0; i < windowSize; ++i) {
        const double t = static_cast<double>(i) / sampleRate;
        double value = 0.0;
        for (int harmonic = 1; harmonic <= 8; ++harmonic) {
            const double gain = (harmonic == 1 ? fundamentalGain : 1.0) / harmonic;
            value += gain * std::sin(2.0 * std::numbers::pi * frequency * harmonic * t + harmonic);
        }
        if (noise > 0.0) {
            value += gaussian(rng);
        }
        samples[i] = static_cast<float>(0.2 * value);
    }
    return samples;
}

double centsBetween(double a, double b)
{
    return 1200.0 * std::log2(a / b);
}
}

class PitchDetectorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testOpenStrings_data()
    {
        QTest::addColumn<double>("sampleRate");
        QTest::addColumn<int>("midi");

        // Lowest supported string (G0 on a 6-string bass in drop tuning) up to high E and beyond.
        for (const double rate : {44100.0, 48000.0}) {
            for (const int midi : {19, 23, 28, 30, 33, 40, 45, 50, 55, 59, 64, 76, 88}) {
                QTest::addRow("%s @ %d", qPrintable(Notes::fullName(midi)), int(rate)) << rate << midi;
            }
        }
    }

    void testOpenStrings()
    {
        QFETCH(double, sampleRate);
        QFETCH(int, midi);

        const double expected = Notes::frequencyFromMidi(midi, 440.0);
        PitchDetector detector(sampleRate, windowSize);
        const auto samples = tone(expected, sampleRate);
        const auto result = detector.detect(samples.data());
        QVERIFY2(std::abs(centsBetween(result.frequency, expected)) < 1.0,
                 qPrintable(QStringLiteral("expected %1 Hz, got %2 Hz").arg(expected).arg(result.frequency)));
        QVERIFY(result.clarity > 0.9);
    }

    void testDetuned()
    {
        // 7 cents flat E2 must read as 7 cents flat, not snap to the note.
        const double expected = Notes::frequencyFromMidi(40 - 0.07, 440.0);
        PitchDetector detector(48000.0, windowSize);
        const auto result = detector.detect(tone(expected, 48000.0).data());
        QVERIFY(std::abs(centsBetween(result.frequency, expected)) < 0.5);
    }

    void testWeakFundamental()
    {
        // Bass pickups can produce a weak fundamental; the detector must not jump an octave up.
        const double expected = Notes::frequencyFromMidi(28, 440.0); // E1
        PitchDetector detector(48000.0, windowSize);
        const auto result = detector.detect(tone(expected, 48000.0, 0.3).data());
        QVERIFY2(std::abs(centsBetween(result.frequency, expected)) < 1.0, qPrintable(QString::number(result.frequency)));
    }

    void testNoisyInput()
    {
        const double expected = Notes::frequencyFromMidi(45, 440.0); // A2
        PitchDetector detector(48000.0, windowSize);
        const auto result = detector.detect(tone(expected, 48000.0, 1.0, 0.1).data());
        QVERIFY(std::abs(centsBetween(result.frequency, expected)) < 2.0);
    }

    void testNoise()
    {
        std::vector<float> samples(windowSize);
        std::mt19937 rng(1);
        std::uniform_real_distribution<float> uniform(-0.5f, 0.5f);
        for (float &sample : samples) {
            sample = uniform(rng);
        }
        PitchDetector detector(48000.0, windowSize);
        QVERIFY(detector.detect(samples.data()).clarity < 0.8);
    }

    void testSilence()
    {
        const std::vector<float> samples(windowSize, 0.0f);
        PitchDetector detector(48000.0, windowSize);
        QCOMPARE(detector.detect(samples.data()).frequency, 0.0);
    }
};

QTEST_GUILESS_MAIN(PitchDetectorTest)
#include "pitchdetectortest.moc"
