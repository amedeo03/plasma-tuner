#include "notes.h"

#include <cmath>

namespace Notes
{
double midiFromFrequency(double frequency, double referencePitch)
{
    return 69.0 + 12.0 * std::log2(frequency / referencePitch);
}

double frequencyFromMidi(double midi, double referencePitch)
{
    return referencePitch * std::exp2((midi - 69.0) / 12.0);
}

QString name(int midi)
{
    static const char16_t *const names[] = {u"C", u"C♯", u"D", u"D♯", u"E", u"F", u"F♯", u"G", u"G♯", u"A", u"A♯", u"B"};
    const int pitchClass = ((midi % 12) + 12) % 12;
    return QString::fromUtf16(names[pitchClass]);
}

int octave(int midi)
{
    // Floor division so negative MIDI numbers still map correctly.
    return static_cast<int>(std::floor(midi / 12.0)) - 1;
}

QString fullName(int midi)
{
    return name(midi) + QString::number(octave(midi));
}
}
