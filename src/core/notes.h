#pragma once

#include <QString>

#include <optional>

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

// Lowest and highest notes accepted by parse(): C0 and C8.
constexpr int lowestNote = 12;
constexpr int highestNote = 108;

struct ParsedNote {
    int midi;
    // Display form, keeping the user's spelling: "Bb1" becomes "B♭1".
    QString label;
};

// Parses a letter A-G, an optional "#" or "b" and an octave, e.g. "E2", "F#1"
// or "Bb0". Returns nothing for malformed notes or notes outside C0-C8.
std::optional<ParsedNote> parse(const QString &text);
}
