#ifndef DAOCLIENT_H
#define DAOCLIENT_H

#include "daoutil.h"

#include <functional>
#include <memory>

#include <QObject>
#include <QPointer>
#include <QMutex>

class DaoClient : public QObject
{
public:
    ~DaoClient();

    static DaoClient &getInstance()
    {
        static DaoClient daoClient;
        return daoClient;
    }

    void add(const QString &dir, const QString &path, const QString &databaseName);

    // 同步执行
    bool execSync(const QString &sql, DaoResultListPtr &result, QString &msg, const QString &databaseName);

    // 同步执行
    bool execBatchSync(const QString &sql, const QMap<QString, QVariant> &filterDatas, const QMap<QString, QVariant> &data, DaoResultListPtr &result, QString &msg, const QString &databaseName);

    void free();

private:
    DaoClient(QObject *parent = nullptr);

    QMap<QString, QSharedPointer<DaoUtil> > m_dbConnectToUtil;

    QMutex m_mutex;

};

#endif // DAOCLIENT_H
