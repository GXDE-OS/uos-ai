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

    enum AnswerOperateType {
        OpenURL = 10001,
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

    virtual void notify(const QString &aiReply, int err) override;
};

class EAiStreamCallback : public EAiCallback
{
public:
    explicit EAiStreamCallback(QObject *caller);
};

class EAiInstructionCallback : public EAiCallback
{
public:
    explicit EAiInstructionCallback(QObject *caller);

    void setInstType(int type);
    int instType();

    virtual void notify(const QString &aiReply, int err) override;

private:
    void sendNotify(const QString &content, int err);
    int inst {-1};

    QString reply;
};

#endif // EAICALLBCK_H
