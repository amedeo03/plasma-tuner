#include "notes.h"

#include <QRegularExpression>

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

std::optional<ParsedNote> parse(const QString &text)
{
    static const QRegularExpression pattern(QStringLiteral("^([A-G])([#b]?)(\\d{1,2})$"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch()) {
        return std::nullopt;
    }
    // Semitones from C for A-G.
    static const int letterOffsets[] = {9, 11, 0, 2, 4, 5, 7};
    const QChar letter = match.captured(1).front();
    const QString accidental = match.captured(2);
    const int octave = match.captured(3).toInt();

    int midi = (octave + 1) * 12 + letterOffsets[letter.unicode() - u'A'];
    QString label = letter;
    if (accidental == u'#') {
        ++midi;
        label += u'♯';
    } else if (accidental == u'b') {
        --midi;
        label += u'♭';
    }
    if (midi < lowestNote || midi > highestNote) {
        return std::nullopt;
    }
    return ParsedNote{midi, label + QString::number(octave)};
}
}
