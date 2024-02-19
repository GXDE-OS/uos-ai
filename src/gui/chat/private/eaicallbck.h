#ifndef EAICALLBCK_H
#define EAICALLBCK_H

#include <QObject>

class EAiCallback
{
public:
    explicit EAiCallback(QObject *caller);
    virtual ~EAiCallback();

    enum DataMode {
        None,
        CacheMode = 1,
        StreamMode,
        MaxMode,
    };

    bool isStreamMode();
    void setOwner(QObject *caller);
    QObject *getOwner();
    void setNotifier(QString notifyName);
    void setDataMode(DataMode m);
    void setOp(int act);

    virtual void notify(const QString &aiReply, int err);

protected:
    QObject *owner {nullptr};
    QString notifier {""};

    DataMode mode {CacheMode};

    int op {0};
};

class EAiCacheCallback : public EAiCallback
{
public:
    explicit EAiCacheCallback(QObject *caller);
};

class EAiStreamCallback : public EAiCallback
{
public:
    explicit EAiStreamCallback(QObject *caller);
};

#endif // EAICALLBCK_H
