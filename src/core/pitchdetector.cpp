#include "pitchdetector.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
// Peaks at least this fraction of the highest one are accepted, the first
// (shortest lag) winning. Avoids picking a sub-harmonic.
constexpr double peakThreshold = 0.9;

// In-place iterative radix-2 FFT. inverse=true computes the unscaled inverse.
void fft(std::vector<std::complex<float>> &data, bool inverse)
{
    const std::size_t n = data.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }
    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double angle = 2.0 * std::numbers::pi / static_cast<double>(len) * (inverse ? 1.0 : -1.0);
        const std::complex<double> step(std::cos(angle), std::sin(angle));
        for (std::size_t start = 0; start < n; start += len) {
            std::complex<double> w(1.0, 0.0);
            for (std::size_t k = 0; k < len / 2; ++k) {
                const std::complex<float> wf(static_cast<float>(w.real()), static_cast<float>(w.imag()));
                const std::complex<float> even = data[start + k];
                const std::complex<float> odd = data[start + k + len / 2] * wf;
                data[start + k] = even + odd;
                data[start + k + len / 2] = even - odd;
                w *= step;
            }
        }
    }
}
}

PitchDetector::PitchDetector(double sampleRate, std::size_t windowSize, double minFrequency, double maxFrequency)
    : m_sampleRate(sampleRate)
    , m_windowSize(windowSize)
    , m_minLag(std::max<std::size_t>(2, static_cast<std::size_t>(sampleRate / maxFrequency)))
    , m_maxLag(std::min(windowSize / 2, static_cast<std::size_t>(sampleRate / minFrequency) + 1))
    , m_spectrum(windowSize * 2)
    , m_nsdf(windowSize / 2 + 2)
{
}

std::size_t PitchDetector::windowSize() const
{
    return m_windowSize;
}

double PitchDetector::sampleRate() const
{
    return m_sampleRate;
}

PitchDetector::Result PitchDetector::detect(const float *samples)
{
    const std::size_t n = m_windowSize;

    // Autocorrelation r(tau) via zero-padded FFT: IFFT(|FFT(x)|^2).
    std::fill(m_spectrum.begin(), m_spectrum.end(), std::complex<float>());
    for (std::size_t i = 0; i < n; ++i) {
        m_spectrum[i] = samples[i];
    }
    fft(m_spectrum, false);
    for (auto &bin : m_spectrum) {
        bin = std::norm(bin);
    }
    fft(m_spectrum, true);
    const float scale = 1.0f / static_cast<float>(m_spectrum.size());

    // NSDF(tau) = 2 r(tau) / m(tau), where m(tau) = sum x[j]^2 + x[j+tau]^2
    // over the overlapping part, updated incrementally.
    double m = 2.0 * m_spectrum[0].real() * scale;
    if (m <= 0.0) {
        return {};
    }
    const std::size_t lastLag = m_maxLag + 1;
    for (std::size_t tau = 0; tau <= lastLag && tau < n; ++tau) {
        m_nsdf[tau] = m > 0.0 ? static_cast<float>(2.0 * m_spectrum[tau].real() * scale / m) : 0.0f;
        m -= double(samples[tau]) * samples[tau] + double(samples[n - 1 - tau]) * samples[n - 1 - tau];
    }

    // Key maxima: the highest point of each positive lobe after the first
    // negative-going zero crossing.
    std::size_t tau = 1;
    while (tau < lastLag && m_nsdf[tau] > 0.0f) {
        ++tau;
    }
    std::vector<std::size_t> peaks;
    while (tau < lastLag) {
        while (tau < lastLag && m_nsdf[tau] <= 0.0f) {
            ++tau;
        }
        std::size_t best = tau;
        while (tau < lastLag && m_nsdf[tau] > 0.0f) {
            if (m_nsdf[tau] > m_nsdf[best]) {
                best = tau;
            }
            ++tau;
        }
        // A lobe cut off by the lag limit has no reliable maximum.
        if (tau < lastLag && best >= m_minLag && best <= m_maxLag) {
            peaks.push_back(best);
        }
    }
    if (peaks.empty()) {
        return {};
    }

    float highest = 0.0f;
    for (std::size_t peak : peaks) {
        highest = std::max(highest, m_nsdf[peak]);
    }
    const float threshold = static_cast<float>(peakThreshold) * highest;
    const std::size_t chosen = *std::find_if(peaks.begin(), peaks.end(), [&](std::size_t peak) {
        return m_nsdf[peak] >= threshold;
    });

    // Parabolic interpolation around the peak for sub-sample precision.
    const double left = m_nsdf[chosen - 1];
    const double centre = m_nsdf[chosen];
    const double right = m_nsdf[chosen + 1];
    const double denominator = left - 2.0 * centre + right;
    double offset = 0.0;
    double clarity = centre;
    if (denominator != 0.0) {
        offset = 0.5 * (left - right) / denominator;
        clarity = centre - 0.25 * (left - right) * offset;
    }
    const double lag = static_cast<double>(chosen) + offset;
    return {m_sampleRate / lag, std::clamp(clarity, 0.0, 1.0)};
}
