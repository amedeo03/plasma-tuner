#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariantList>

// Exposes the instrument and tuning catalog to QML.
class TuningCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit TuningCatalog(QObject *parent = nullptr);

    // [{value, text}] for each instrument.
    Q_INVOKABLE QVariantList instruments() const;
    // Supported string counts, the default first.
    Q_INVOKABLE QVariantList stringCounts(int instrument) const;
    // [{text, notes: [{midi, name, octave}]}], notes from the lowest string.
    Q_INVOKABLE QVariantList tunings(int instrument, int strings) const;
};
