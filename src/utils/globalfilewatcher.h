#ifndef GLOBALFILEWATCHER_H
#define GLOBALFILEWATCHER_H

#include <QObject>
#include <QSet>

class QFileSystemWatcher;
namespace uos_ai {

class FileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit FileWatcher(QObject *parent = nullptr);
    ~FileWatcher();
    bool addPath(const QString &file);
    QStringList addPaths(const QStringList &files);
    bool removePath(const QString &file);
    QStringList removePaths(const QStringList &files);
    inline QStringList allPaths() const {
        return paths;
    }
Q_SIGNALS:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);
private:
    QStringList paths;
};

class GlobalFileWatcher : public QObject
{
    Q_OBJECT
    friend class FileWatcher;
public:
    static GlobalFileWatcher *instance();
    void registerWatcher(FileWatcher *);
    void removeWatcher(FileWatcher *);
    bool isRegistered(FileWatcher *);
protected:
    bool addPath(const QString &file);
    bool removePath(const QString &file);
private Q_SLOTS:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);
private:
     explicit GlobalFileWatcher(QObject *parent = nullptr);
private:
    QFileSystemWatcher *fileWatcher = nullptr;
    QSet<FileWatcher *> wactherList;
    QStringList paths;
};

}

#define GFWatcher GlobalFileWatcher::instance

#endif // GLOBALFILEWATCHER_H
