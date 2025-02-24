// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "fileinfothread_p.h"
#include <qdiriterator.h>
#include <qpointer.h>
#include <qtimer.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

FileInfoThread::FileInfoThread(QObject *parent)
    : QThread(parent),
      abort(false),
      scanPending(false),
#if QT_CONFIG(filesystemwatcher)
      watcher(nullptr),
#endif
      sortFlags(QDir::Name),
      needUpdate(true),
      folderUpdate(false),
      sortUpdate(false),
      showFiles(true),
      showDirs(true),
      showDirsFirst(false),
      showDotAndDotDot(false),
      showHidden(false),
      showOnlyReadable(false),
      caseSensitive(true)
{
#if QT_CONFIG(filesystemwatcher)
    watcher = new QFileSystemWatcher(this);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(updateFile(QString)));
#endif // filesystemwatcher
}

FileInfoThread::~FileInfoThread()
{
    QMutexLocker locker(&mutex);
    abort = true;
    condition.wakeOne();
    locker.unlock();
    wait();
}

void FileInfoThread::clear()
{
    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    watcher->removePaths(watcher->files());
    watcher->removePaths(watcher->directories());
#endif
}

void FileInfoThread::removePath(const QString &path)
{
    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    if (!path.startsWith(QLatin1Char(':')))
        watcher->removePath(path);
#else
    Q_UNUSED(path);
#endif
    currentPath.clear();
}

void FileInfoThread::setPath(const QString &path)
{
    Q_ASSERT(!path.isEmpty());

    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    if (!path.startsWith(QLatin1Char(':')))
        watcher->addPath(path);
#endif
    currentPath = path;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setRootPath(const QString &path)
{
    Q_ASSERT(!path.isEmpty());

    QMutexLocker locker(&mutex);
    rootPath = path;
}

#if QT_CONFIG(filesystemwatcher)
void FileInfoThread::dirChanged(const QString &directoryPath)
{
    Q_UNUSED(directoryPath);
    QMutexLocker locker(&mutex);
    folderUpdate = true;
    initiateScan();
}
#endif

void FileInfoThread::setSortFlags(QDir::SortFlags flags)
{
    QMutexLocker locker(&mutex);
    sortFlags = flags;
    sortUpdate = true;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setNameFilters(const QStringList & filters)
{
    QMutexLocker locker(&mutex);
    nameFilters = filters;
    folderUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowFiles(bool show)
{
    QMutexLocker locker(&mutex);
    showFiles = show;
    folderUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowDirs(bool showFolders)
{
    QMutexLocker locker(&mutex);
    showDirs = showFolders;
    folderUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowDirsFirst(bool show)
{
    QMutexLocker locker(&mutex);
    showDirsFirst = show;
    folderUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowDotAndDotDot(bool on)
{
    QMutexLocker locker(&mutex);
    showDotAndDotDot = on;
    folderUpdate = true;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowHidden(bool on)
{
    QMutexLocker locker(&mutex);
    showHidden = on;
    folderUpdate = true;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowOnlyReadable(bool on)
{
    QMutexLocker locker(&mutex);
    showOnlyReadable = on;
    folderUpdate = true;
    initiateScan();
}

void FileInfoThread::setCaseSensitive(bool on)
{
    QMutexLocker locker(&mutex);
    caseSensitive = on;
    folderUpdate = true;
    initiateScan();
}

#if QT_CONFIG(filesystemwatcher)
void FileInfoThread::updateFile(const QString &path)
{
    Q_UNUSED(path);
    QMutexLocker locker(&mutex);
    folderUpdate = true;
    initiateScan();
}
#endif

void FileInfoThread::run()
{
    forever {
        bool updateFiles = false;
        QMutexLocker locker(&mutex);
        if (abort) {
            return;
        }
        if (currentPath.isEmpty() || !needUpdate) {
            emit statusChanged(currentPath.isEmpty() ? QQuickFolderListModel::Null : QQuickFolderListModel::Ready);
            condition.wait(&mutex);
        }

        if (abort) {
            return;
        }

        if (!currentPath.isEmpty()) {
            updateFiles = true;
            emit statusChanged(QQuickFolderListModel::Loading);
        }
        if (updateFiles)
            getFileInfos(currentPath);
        locker.unlock();
    }
}

void FileInfoThread::runOnce()
{
    if (scanPending)
        return;
    scanPending = true;
    QPointer<FileInfoThread> guardedThis(this);

    auto getFileInfosAsync = [guardedThis](){
        if (!guardedThis)
            return;
        guardedThis->scanPending = false;
        if (guardedThis->currentPath.isEmpty()) {
            emit guardedThis->statusChanged(QQuickFolderListModel::Null);
            return;
        }
        emit guardedThis->statusChanged(QQuickFolderListModel::Loading);
        guardedThis->getFileInfos(guardedThis->currentPath);
        emit guardedThis->statusChanged(QQuickFolderListModel::Ready);
    };

    QTimer::singleShot(0, getFileInfosAsync);
}

void FileInfoThread::initiateScan()
{
#if QT_CONFIG(thread)
    condition.wakeAll();
#else
    runOnce();
#endif
}

void FileInfoThread::getFileInfos(const QString &path)
{
    QDir::Filters filter;
    if (caseSensitive)
        filter = QDir::CaseSensitive;
    if (showFiles)
        filter = filter | QDir::Files;
    if (showDirs)
        filter = filter | QDir::AllDirs | QDir::Drives;
    if (!showDotAndDotDot)
        filter = filter | QDir::NoDot | QDir::NoDotDot;
    else if (path == rootPath)
        filter = filter | QDir::NoDotDot;
    if (showHidden)
        filter = filter | QDir::Hidden;
    if (showOnlyReadable)
        filter = filter | QDir::Readable;
    if (showDirsFirst)
        sortFlags = sortFlags | QDir::DirsFirst;

    QDir currentDir(path, QString(), sortFlags);
    QList<FileProperty> filePropertyList;

    const QFileInfoList fileInfoList = currentDir.entryInfoList(nameFilters, filter, sortFlags);

    if (!fileInfoList.isEmpty()) {
        filePropertyList.reserve(fileInfoList.count());
        for (const QFileInfo &info : fileInfoList) {
            //qDebug() << "Adding file : " << info.fileName() << "to list ";
            filePropertyList << FileProperty(info);
        }
        if (folderUpdate) {
            int fromIndex = 0;
            int toIndex = currentFileList.size()-1;
            findChangeRange(filePropertyList, fromIndex, toIndex);
            folderUpdate = false;
            currentFileList = filePropertyList;
            //qDebug() << "emit directoryUpdated : " << fromIndex << " " << toIndex;
            emit directoryUpdated(path, filePropertyList, fromIndex, toIndex);
        } else {
            currentFileList = filePropertyList;
            if (sortUpdate) {
                emit sortFinished(filePropertyList);
                sortUpdate = false;
            } else
                emit directoryChanged(path, filePropertyList);
        }
    } else {
        // The directory is empty
        if (folderUpdate) {
            int fromIndex = 0;
            int toIndex = currentFileList.size()-1;
            folderUpdate = false;
            currentFileList.clear();
            emit directoryUpdated(path, filePropertyList, fromIndex, toIndex);
        } else {
            currentFileList.clear();
            emit directoryChanged(path, filePropertyList);
        }
    }
    needUpdate = false;
}

void FileInfoThread::findChangeRange(const QList<FileProperty> &list, int &fromIndex, int &toIndex)
{
    if (currentFileList.size() == 0) {
        fromIndex = 0;
        toIndex = list.size();
        return;
    }

    int i;
    int listSize = list.size() < currentFileList.size() ? list.size() : currentFileList.size();
    bool changeFound = false;

    for (i=0; i < listSize; i++) {
        if (list.at(i) != currentFileList.at(i)) {
            changeFound = true;
            break;
        }
    }

    if (changeFound)
        fromIndex = i;
    else
        fromIndex = i-1;

    // For now I let the rest of the list be updated..
    toIndex = list.size() > currentFileList.size() ? list.size() - 1 : currentFileList.size() - 1;
}

QT_END_NAMESPACE

#include "moc_fileinfothread_p.cpp"
