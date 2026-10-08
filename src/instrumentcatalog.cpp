#include "instrumentcatalog.h"

#include "core/instrumentconfig.h"

#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

using namespace std::chrono_literals;

namespace
{
QVariantList toVariant(const QList<InstrumentConfig::Instrument> &instruments)
{
    QVariantList list;
    for (const InstrumentConfig::Instrument &instrument : instruments) {
        QVariantList tunings;
        for (const InstrumentConfig::Tuning &tuning : instrument.tunings) {
            QVariantList notes;
            for (qsizetype i = 0; i < tuning.notes.size(); ++i) {
                notes.append(QVariantMap{{QStringLiteral("midi"), tuning.notes.at(i)}, {QStringLiteral("label"), tuning.labels.at(i)}});
            }
            tunings.append(QVariantMap{
                {QStringLiteral("name"), tuning.name},
                {QStringLiteral("valid"), tuning.isValid()},
                {QStringLiteral("error"), tuning.error},
                {QStringLiteral("notes"), notes},
            });
        }
        list.append(QVariantMap{
            {QStringLiteral("name"), instrument.name},
            {QStringLiteral("valid"), instrument.isValid()},
            {QStringLiteral("error"), instrument.error},
            {QStringLiteral("tunings"), tunings},
        });
    }
    return list;
}
}

InstrumentCatalog::InstrumentCatalog(QObject *parent)
    : QObject(parent)
    , m_path(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/plasma-tuner/instruments.json"))
{
    // Editors often save by replacing the file, which fires several change
    // notifications in a row: reload once things settle.
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(200ms);
    connect(&m_reloadTimer, &QTimer::timeout, this, [this] {
        load();
        watch();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_reloadTimer, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_reloadTimer, qOverload<>(&QTimer::start));

    ensureFileExists();
    load();
    watch();
}

QVariantList InstrumentCatalog::instruments() const
{
    return m_instruments;
}

QString InstrumentCatalog::fileError() const
{
    return m_fileError;
}

QUrl InstrumentCatalog::fileUrl() const
{
    return QUrl::fromLocalFile(m_path);
}

void InstrumentCatalog::ensureFileExists()
{
    if (QFileInfo::exists(m_path)) {
        return;
    }
    QDir().mkpath(QFileInfo(m_path).path());
    QSaveFile file(m_path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(InstrumentConfig::defaultJson());
        file.commit();
    }
}

void InstrumentCatalog::load()
{
    InstrumentConfig::Config config;
    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        config = InstrumentConfig::parse(file.readAll());
    } else {
        config.error = i18nc("@info", "The file could not be read.");
    }

    QString fileError;
    if (!config.error.isEmpty()) {
        fileError = i18nc("@info %1 is a file path, %2 the problem", "Error in %1: %2 Using the built-in instruments.", m_path, config.error);
        config = InstrumentConfig::parse(InstrumentConfig::defaultJson());
    }

    m_instruments = toVariant(config.instruments);
    m_fileError = fileError;
    Q_EMIT instrumentsChanged();
}

void InstrumentCatalog::watch()
{
    // Watch the directory too, so a deleted or replaced file is noticed.
    const QString directory = QFileInfo(m_path).path();
    if (!m_watcher.directories().contains(directory)) {
        m_watcher.addPath(directory);
    }
    if (QFileInfo::exists(m_path) && !m_watcher.files().contains(m_path)) {
        m_watcher.addPath(m_path);
    }
}
