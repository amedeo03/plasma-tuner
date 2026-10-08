#include "tunings.h"

#include "notes.h"

#include <KLocalizedString>

#include <iterator>

namespace
{
struct Preset {
    int transpose; // semitones relative to standard tuning
    bool drop; // lowest string lowered one more whole step
};

// E standard, Drop D, D standard, Drop C, C standard
constexpr Preset presets[] = {
    {0, false},
    {0, true},
    {-2, false},
    {-2, true},
    {-4, false},
};

QList<int> standardNotes(Tunings::Instrument instrument, int strings)
{
    // Extended-range instruments add strings below (and, for 6-string bass, above) the standard set.
    switch (instrument) {
    case Tunings::Instrument::Guitar:
        switch (strings) {
        case 6:
            return {40, 45, 50, 55, 59, 64}; // E2 A2 D3 G3 B3 E4
        case 7:
            return {35, 40, 45, 50, 55, 59, 64}; // B1 ...
        case 8:
            return {30, 35, 40, 45, 50, 55, 59, 64}; // F#1 B1 ...
        }
        break;
    case Tunings::Instrument::Bass:
        switch (strings) {
        case 4:
            return {28, 33, 38, 43}; // E1 A1 D2 G2
        case 5:
            return {23, 28, 33, 38, 43}; // B0 ...
        case 6:
            return {23, 28, 33, 38, 43, 48}; // B0 ... C3
        }
        break;
    }
    return {};
}
}

namespace Tunings
{
QList<Instrument> instruments()
{
    return {Instrument::Guitar, Instrument::Bass};
}

QString instrumentName(Instrument instrument)
{
    switch (instrument) {
    case Instrument::Guitar:
        return i18nc("@item:inlistbox instrument", "Electric guitar");
    case Instrument::Bass:
        return i18nc("@item:inlistbox instrument", "Electric bass");
    }
    return {};
}

QList<int> stringCounts(Instrument instrument)
{
    switch (instrument) {
    case Instrument::Guitar:
        return {6, 7, 8};
    case Instrument::Bass:
        return {4, 5, 6};
    }
    return {};
}

int tuningCount()
{
    return static_cast<int>(std::size(presets));
}

QList<int> tuningNotes(Instrument instrument, int strings, int tuning)
{
    if (tuning < 0 || tuning >= tuningCount()) {
        return {};
    }
    QList<int> notes = standardNotes(instrument, strings);
    if (notes.isEmpty()) {
        return {};
    }
    const Preset &preset = presets[tuning];
    for (int &note : notes) {
        note += preset.transpose;
    }
    if (preset.drop) {
        notes.first() -= 2;
    }
    return notes;
}

QString tuningName(Instrument instrument, int strings, int tuning)
{
    const QList<int> notes = tuningNotes(instrument, strings, tuning);
    if (notes.isEmpty()) {
        return {};
    }
    const Preset &preset = presets[tuning];
    if (preset.drop) {
        // Drop tunings are named after the lowest string: Drop D on a 6-string, Drop A on a 7-string.
        return i18nc("@item:inlistbox tuning, %1 is a note name", "Drop %1", Notes::name(notes.first()));
    }
    // Standard tunings are named after the transposed E string, whatever the string count (E = pitch class 4).
    return i18nc("@item:inlistbox tuning, %1 is a note name", "%1 standard", Notes::name(4 + preset.transpose));
}
}
