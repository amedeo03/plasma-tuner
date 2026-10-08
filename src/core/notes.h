#pragma once

#include <QString>

// Equal-temperament helpers. Notes are identified by MIDI number (A4 = 69).
namespace Notes
{
// Fractional MIDI note for a frequency, relative to the given A4 reference.
double midiFromFrequency(double frequency, double referencePitch);
double frequencyFromMidi(double midi, double referencePitch);

// Pitch class name using sharps, e.g. "C", "F♯".
QString name(int midi);
// Scientific pitch notation octave (C4 = middle C = MIDI 60).
int octave(int midi);
// Name and octave, e.g. "E2".
QString fullName(int midi);
}
