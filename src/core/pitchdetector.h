#pragma once

#include <complex>
#include <cstddef>
#include <vector>

// Monophonic pitch detector based on the McLeod Pitch Method (normalized
// square difference function, computed through an FFT autocorrelation).
class PitchDetector
{
public:
    struct Result {
        double frequency = 0.0; // Hz, 0 when no pitch was found
        double clarity = 0.0; // 0..1, how periodic the signal is
    };

    // windowSize must be a power of two; it bounds the lowest detectable
    // frequency to roughly 2 * sampleRate / windowSize.
    PitchDetector(double sampleRate, std::size_t windowSize, double minFrequency = 20.0, double maxFrequency = 1500.0);

    std::size_t windowSize() const;
    double sampleRate() const;

    // Analyses exactly windowSize() samples.
    Result detect(const float *samples);

private:
    double m_sampleRate;
    std::size_t m_windowSize;
    std::size_t m_minLag;
    std::size_t m_maxLag;
    std::vector<std::complex<float>> m_spectrum;
    std::vector<float> m_nsdf;
};
