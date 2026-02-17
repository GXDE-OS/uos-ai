#include "globalfilewatcher.h"

#include <QThread>
#include <QApplication>
#include <QFileSystemWatcher>
#include <QDebug>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logUtils)

using namespace uos_ai;

GlobalFileWatcher *GlobalFileWatcher::instance()
{
    static GlobalFileWatcher ins;
    return &ins;
}

void GlobalFileWatcher::registerWatcher(FileWatcher *w)
{
    if (!w || wactherList.contains(w))
        return;

    wactherList.insert(w);
}

void GlobalFileWatcher::removeWatcher(FileWatcher *w)
{
    wactherList.remove(w);
}

bool GlobalFileWatcher::isRegistered(FileWatcher *w)
{
    return wactherList.contains(w);
}

bool GlobalFileWatcher::addPath(const QString &file)
{
    if (!fileWatcher->addPath(file))
        return false;

    paths.append(file);
    return true;
}

bool GlobalFileWatcher::removePath(const QString &file)
{
    paths.removeOne(file);
    if (!paths.contains(file))
        fileWatcher->removePath(file);

    return true;
}

void GlobalFileWatcher::onFileChanged(const QString &path)
{
    for (FileWatcher *w : wactherList) {
        if (w->allPaths().contains(path))
            emit w->fileChanged(path);
    }
}

void GlobalFileWatcher::onDirectoryChanged(const QString &path)
{
    for (FileWatcher *w : wactherList) {
        if (w->allPaths().contains(path))
            emit w->directoryChanged(path);
    }
}

GlobalFileWatcher::GlobalFileWatcher(QObject *parent) : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    fileWatcher = new QFileSystemWatcher(this);

    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &GlobalFileWatcher::onFileChanged);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &GlobalFileWatcher::onFileChanged);
}

FileWatcher::FileWatcher(QObject *parent) : QObject(parent)
{
    if (QThread::currentThread() != GFWatcher()->thread()) {
        qCCritical(logUtils) << "Can not add watcher to global instance with different thread.";
    }

    GFWatcher()->registerWatcher(this);
}

FileWatcher::~FileWatcher()
{
    removePaths(paths);
    GFWatcher()->removeWatcher(this);
}

bool FileWatcher::addPath(const QString &file)
{
    if (!GFWatcher()->isRegistered(this))
        return false;

    if (!GFWatcher()->addPath(file))
        return false;

    paths.append(file);
    return true;
}

QStringList FileWatcher::addPaths(const QStringList &files)
{
    QStringList ret;
    for (const QString &file : files) {
        if (!GFWatcher()->addPath(file))
            continue;
        ret.append(file);
        paths.append(file);
    }

    return ret;
}

bool FileWatcher::removePath(const QString &file)
{
    if (!GFWatcher()->isRegistered(this))
        return false;

    GFWatcher()->removePath(file);
    paths.removeOne(file);
    return true;
}

QStringList FileWatcher::removePaths(const QStringList &files)
{
    QStringList ret;
    for (const QString &file : files) {
        GFWatcher()->removePath(file);
        ret.append(file);
        paths.removeOne(file);
    }

    return ret;
}
