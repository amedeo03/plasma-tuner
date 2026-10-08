#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

// Parsing and validation of the user's instruments file:
//
// { "instruments": [ { "name": "...", "stringCount": 6,
//     "tunings": [ { "name": "...", "stringNotes": ["E2", "A2", ...] } ] } ] }
//
// Invalid instruments and tunings are kept, with an error message, so the UI
// can list them and explain what is wrong when they are selected.
namespace InstrumentConfig
{
constexpr int maximumStrings = 12;

struct Tuning {
    QString name;
    QList<int> notes; // MIDI notes, lowest string first
    QStringList labels; // display names matching notes
    QString error; // empty when valid

    bool isValid() const
    {
        return error.isEmpty();
    }
};

struct Instrument {
    QString name;
    int stringCount = 0;
    QList<Tuning> tunings;
    QString error; // problems with the instrument itself, not its tunings

    bool isValid() const
    {
        return error.isEmpty();
    }
};

struct Config {
    QList<Instrument> instruments;
    // Set when the file as a whole is unusable (syntax error, wrong structure);
    // instruments is then empty.
    QString error;
};

Config parse(const QByteArray &json);

// The built-in instruments, also written as the initial user file.
QByteArray defaultJson();
}
