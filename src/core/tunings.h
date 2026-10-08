#pragma once

#include <QList>
#include <QString>

// Catalog of instruments and tunings. The tuner itself is chromatic: tunings
// only tell the user which notes each string should be tuned to.
namespace Tunings
{
enum class Instrument {
    Guitar = 0,
    Bass = 1,
};

QList<Instrument> instruments();
QString instrumentName(Instrument instrument);

// Supported string counts, the first one being the default.
QList<int> stringCounts(Instrument instrument);

// Tuning presets, valid for every instrument and string count.
int tuningCount();
// Display name, e.g. "E standard" or "Drop A" (named after the dropped string).
QString tuningName(Instrument instrument, int strings, int tuning);
// MIDI notes from the lowest to the highest string. Empty for invalid input.
QList<int> tuningNotes(Instrument instrument, int strings, int tuning);
}
