
#include "workspacemodel.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <cstdint>
#include <limits>
#include <qtreewidget.h>

// #define BLOCK_ALLOC_DEBUG_MSG

WorkspaceModel::WorkspaceModel(QObject *parent) : QObject(parent) {
    QSettings settings;

    // Initialize cache file mapping
    auto backingFileName = settings.value("CacheFile/Path").toString();
    m_backingStore.setFileName(backingFileName);

    QFileInfo fileInfo(backingFileName);
    if (backingFileName.isEmpty() || fileInfo.isDir() || !fileInfo.isWritable() ||
        !m_backingStore.open(QFile::ReadWrite)) {
        // User defined file is not usable, try default temp path
        auto tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        fileInfo.setFile(tempDir);
        if (!fileInfo.dir().exists()) {
            QDir().mkpath(tempDir);
        }
        m_backingStore.setFileName(QDir::toNativeSeparators(tempDir + QDir::separator() + "probescope_cache"));
        if (!m_backingStore.open(QFile::ReadWrite)) {
            do {
                auto choice = QMessageBox::warning(
                    nullptr, tr("Cannot open cache file"),
                    tr("Cache file %1 cannot be opened with read/write access.\nDo you wish to choose "
                       "another place for cache file?")
                        .arg(m_backingStore.fileName()),
                    QMessageBox::Yes | QMessageBox::Abort, QMessageBox::Yes);

                switch (choice) {
                    default:
                    case QMessageBox::Abort: exit(1); return;

                    case QMessageBox::Yes: {
                        auto chosenFile = QFileDialog::getSaveFileName(
                            nullptr, tr("Select where to save cache file..."), fileInfo.canonicalPath());

                        if (chosenFile.isEmpty())
                            continue;

                        m_backingStore.setFileName(chosenFile);
                        break; // Drops to the loop criteria of while
                    }
                }
            } while (!m_backingStore.open(QFile::ReadWrite));
        }
    }

    bool isOk = false;
    auto mappingBlockLimit = settings.value("CacheFile/BlockCountLimit").toULongLong(&isOk);
    if (!isOk) {
        mappingBlockLimit = 32; // Failsafe option
    }
    m_backingStore.setBlockCountLimit(mappingBlockLimit);

    // Reuse clear routine as initialization here
    m_backingStore.clearStorage();

    if (!m_backingStore.storageMapping()) {
        QMessageBox::critical(nullptr, tr("Cannot map cache file"),
                              tr("Cache file cannot be mapped to system memory. ProbeScope will now quit."));
        exit(1);
    }

    m_symbolBackend = new SymbolBackend(this);
}

WorkspaceModel::~WorkspaceModel() {}

Result<void, SymbolBackend::Error> WorkspaceModel::loadSymbolFile(QString path) {
    auto result = m_symbolBackend->switchSymbolFile(path);
    // TODO: discard watch entries

    if (result.isOk()) {
        return Ok();
    } else {
        return Err(result.unwrapErr());
    }
}

Result<QPair<uint64_t, WorkspaceModel::WatchEntryConfigurations>, WorkspaceModel::Error>
    WorkspaceModel::addWatchEntry(QString expression) {
    //

    return Err(Error::NoError);
}

/***************************************** INTERNAL UTILS *****************************************/
