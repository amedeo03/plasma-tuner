// SPDX-FileCopyrightText: 2026 amedeo03 <amedeomarino03@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "tuningcatalog.h"

#include "core/notes.h"
#include "core/tunings.h"

TuningCatalog::TuningCatalog(QObject *parent)
    : QObject(parent)
{
}

QVariantList TuningCatalog::instruments() const
{
    QVariantList list;
    for (const Tunings::Instrument instrument : Tunings::instruments()) {
        list.append(QVariantMap{{QStringLiteral("value"), static_cast<int>(instrument)}, {QStringLiteral("text"), Tunings::instrumentName(instrument)}});
    }
    return list;
}

QVariantList TuningCatalog::stringCounts(int instrument) const
{
    QVariantList list;
    for (const int count : Tunings::stringCounts(static_cast<Tunings::Instrument>(instrument))) {
        list.append(count);
    }
    return list;
}

QVariantList TuningCatalog::tunings(int instrument, int strings) const
{
    const auto type = static_cast<Tunings::Instrument>(instrument);
    QVariantList list;
    for (int tuning = 0; tuning < Tunings::tuningCount(); ++tuning) {
        QVariantList notes;
        for (const int midi : Tunings::tuningNotes(type, strings, tuning)) {
            notes.append(QVariantMap{{QStringLiteral("midi"), midi}, {QStringLiteral("name"), Notes::name(midi)}, {QStringLiteral("octave"), Notes::octave(midi)}});
        }
        if (notes.isEmpty()) {
            continue;
        }
        list.append(QVariantMap{{QStringLiteral("text"), Tunings::tuningName(type, strings, tuning)}, {QStringLiteral("notes"), notes}});
    }
    return list;
}
