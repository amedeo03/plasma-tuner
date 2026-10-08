#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

// The user's instruments file (~/.config/plasma-tuner/instruments.json),
// created from the built-in instruments on first use and reloaded on change.
class InstrumentCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // [{name, valid, error, tunings: [{name, valid, error, notes: [{midi, label}]}]}]
    Q_PROPERTY(QVariantList instruments READ instruments NOTIFY instrumentsChanged)
    // Set when the file can't be used at all; the built-in instruments are shown instead.
    Q_PROPERTY(QString fileError READ fileError NOTIFY instrumentsChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl CONSTANT)

public:
    explicit InstrumentCatalog(QObject *parent = nullptr);

    QVariantList instruments() const;
    QString fileError() const;
    QUrl fileUrl() const;

Q_SIGNALS:
    void instrumentsChanged();

private:
    void ensureFileExists();
    void load();
    void watch();

    QString m_path;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
    QVariantList m_instruments;
    QString m_fileError;
};
