#include "instrumentconfig.h"

#include "notes.h"

#include <KLocalizedString>

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace
{
// Compact JSON for a value, to quote it back in error messages.
QString describe(const QJsonValue &value)
{
    if (value.isUndefined()) {
        return i18nc("@info a missing JSON value", "nothing");
    }
    if (value.isString()) {
        return QStringLiteral("\"%1\"").arg(value.toString());
    }
    QJsonArray wrapper{value};
    QString text = QString::fromUtf8(QJsonDocument(wrapper).toJson(QJsonDocument::Compact));
    return text.mid(1, text.size() - 2);
}

int lineAt(const QByteArray &json, int offset)
{
    return static_cast<int>(json.left(offset).count('\n')) + 1;
}

// Entries sharing a name can't be told apart once selected: mark them all.
template<typename T>
void markDuplicates(QList<T> &entries, const KLocalizedString &message)
{
    QHash<QString, int> counts;
    for (const T &entry : entries) {
        if (!entry.name.isEmpty()) {
            ++counts[entry.name];
        }
    }
    for (T &entry : entries) {
        if (entry.error.isEmpty() && counts.value(entry.name) > 1) {
            entry.error = message.subs(entry.name).toString();
        }
    }
}

InstrumentConfig::Tuning parseTuning(const QJsonValue &value, int index, int stringCount)
{
    InstrumentConfig::Tuning tuning;
    if (!value.isObject()) {
        tuning.name = i18nc("@item:inlistbox placeholder for a broken tuning", "Tuning %1", index + 1);
        tuning.error = i18nc("@info", "This tuning is not a JSON object.");
        return tuning;
    }
    const QJsonObject object = value.toObject();

    tuning.name = object.value(u"name").toString().trimmed();
    if (tuning.name.isEmpty()) {
        tuning.name = i18nc("@item:inlistbox placeholder for an unnamed tuning", "Tuning %1", index + 1);
        tuning.error = i18nc("@info", "This tuning has no \"name\".");
        return tuning;
    }

    const QJsonValue notesValue = object.value(u"stringNotes");
    if (!notesValue.isArray()) {
        tuning.error = i18nc("@info", "\"stringNotes\" must be an array of notes, found %1.", describe(notesValue));
        return tuning;
    }
    const QJsonArray notes = notesValue.toArray();
    if (notes.size() != stringCount) {
        tuning.error = i18ncp("@info %2 is the number of notes",
                              "The instrument has %1 string but \"stringNotes\" lists %2 notes.",
                              "The instrument has %1 strings but \"stringNotes\" lists %2 notes.",
                              stringCount,
                              notes.size());
        return tuning;
    }
    for (qsizetype i = 0; i < notes.size(); ++i) {
        const std::optional<Notes::ParsedNote> note = notes.at(i).isString() ? Notes::parse(notes.at(i).toString()) : std::nullopt;
        if (!note) {
            tuning.error = i18nc("@info %1 is the position in the list, %2 the faulty value",
                                 "Note %1 (%2) is not valid. Write a letter A–G, an optional # or b, and an octave from C0 to C8, e.g. E2, F#1 or Bb0.",
                                 i + 1,
                                 describe(notes.at(i)));
            tuning.notes.clear();
            tuning.labels.clear();
            return tuning;
        }
        tuning.notes.append(note->midi);
        tuning.labels.append(note->label);
    }
    return tuning;
}

InstrumentConfig::Instrument parseInstrument(const QJsonValue &value, int index)
{
    InstrumentConfig::Instrument instrument;
    if (!value.isObject()) {
        instrument.name = i18nc("@item:inlistbox placeholder for a broken instrument", "Instrument %1", index + 1);
        instrument.error = i18nc("@info", "This instrument is not a JSON object.");
        return instrument;
    }
    const QJsonObject object = value.toObject();

    instrument.name = object.value(u"name").toString().trimmed();
    if (instrument.name.isEmpty()) {
        instrument.name = i18nc("@item:inlistbox placeholder for an unnamed instrument", "Instrument %1", index + 1);
        instrument.error = i18nc("@info", "This instrument has no \"name\".");
        return instrument;
    }

    const QJsonValue countValue = object.value(u"stringCount");
    const double count = countValue.toDouble(-1.0);
    if (!countValue.isDouble() || count != std::floor(count) || count < 1 || count > InstrumentConfig::maximumStrings) {
        instrument.error =
            i18nc("@info", "\"stringCount\" must be a whole number from 1 to %1, found %2.", InstrumentConfig::maximumStrings, describe(countValue));
        return instrument;
    }
    instrument.stringCount = static_cast<int>(count);

    const QJsonValue tuningsValue = object.value(u"tunings");
    if (!tuningsValue.isArray() || tuningsValue.toArray().isEmpty()) {
        instrument.error = i18nc("@info", "\"tunings\" must be a non-empty array of tunings, found %1.", describe(tuningsValue));
        return instrument;
    }
    const QJsonArray tunings = tuningsValue.toArray();
    for (qsizetype i = 0; i < tunings.size(); ++i) {
        instrument.tunings.append(parseTuning(tunings.at(i), static_cast<int>(i), instrument.stringCount));
    }
    markDuplicates(instrument.tunings, ki18nc("@info", "Another tuning of this instrument is also named \"%1\"."));
    return instrument;
}
}

namespace InstrumentConfig
{
Config parse(const QByteArray &json)
{
    Config config;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        config.error = i18nc("@info", "Syntax error on line %1: %2.", lineAt(json, parseError.offset), parseError.errorString());
        return config;
    }
    const QJsonValue instrumentsValue = document.object().value(u"instruments");
    if (!document.isObject() || !instrumentsValue.isArray()) {
        config.error = i18nc("@info", "The file must contain an object with an \"instruments\" array.");
        return config;
    }
    const QJsonArray instruments = instrumentsValue.toArray();
    if (instruments.isEmpty()) {
        config.error = i18nc("@info", "The \"instruments\" array is empty.");
        return config;
    }
    for (qsizetype i = 0; i < instruments.size(); ++i) {
        config.instruments.append(parseInstrument(instruments.at(i), static_cast<int>(i)));
    }
    markDuplicates(config.instruments, ki18nc("@info", "Another instrument is also named \"%1\"."));
    return config;
}

QByteArray defaultJson()
{
    QFile file(QStringLiteral(":/plasmatuner/defaultinstruments.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("Built-in instruments resource is missing");
    }
    return file.readAll();
}
}
