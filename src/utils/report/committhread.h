#ifndef COMMITTHREAD_H
#define COMMITTHREAD_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE

namespace report {

class CommitLog : public QObject
{
    Q_OBJECT
public:
    using InitEventLog = bool (*)(const std::string &, bool);
    using WriteEventLog = void (*)(const std::string &);

    explicit CommitLog(QObject *parent = nullptr);
    ~CommitLog();

public Q_SLOTS:
    void commit(const QString &data);
    bool init();

private:
    InitEventLog m_initEventLog = nullptr;
    WriteEventLog m_writeEventLog = nullptr;
};

}

UOSAI_END_NAMESPACE

#endif   // COMMITTHREAD_H
