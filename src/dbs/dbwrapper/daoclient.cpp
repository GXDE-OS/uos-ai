#include "daoclient.h"

#include <QUuid>
#include <QDir>
#include <QApplication>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>

DaoClient::DaoClient(QObject *parent)
    : QObject(parent)
{

}

DaoClient::~DaoClient()
{

}

void DaoClient::free()
{
}

void DaoClient::add(const QString &dir, const QString &path, const QString &databaseName)
{
    QMutexLocker locker(&m_mutex);
    if (m_dbConnectToUtil.contains(databaseName)) {
        return;
    }

    QDir toolDir;
    toolDir.mkpath(dir);
    QString dbDir = dir;
    QString dbPath = path;

    QString user = "";
    QString passwd = "uos-ai-assistant.db";
    QString driverType = "QSQLITE";

    QString dbDriveName = databaseName;
    QVariantString paramMap;
    paramMap.insert("dbDir", dbDir);
    paramMap.insert("dbPath", dbPath);
    paramMap.insert("user", user);
    paramMap.insert("passwd", passwd);
    paramMap.insert("driverType", driverType);
    paramMap.insert("dbDriveName", dbDriveName);

    auto pUtil = QSharedPointer<DaoUtil>(new DaoUtil());
    pUtil->initDb(dbDir, dbPath, user, passwd, driverType, dbDriveName);
    m_dbConnectToUtil.insert(databaseName, pUtil);
}

bool DaoClient::execSync(const QString &sql, DaoResultListPtr &result, QString &msg, const QString &databaseName)
{
    QMutexLocker locker(&m_mutex);
    bool execSucces = false;
    auto iterUtil = m_dbConnectToUtil.find(databaseName);
    if (iterUtil == m_dbConnectToUtil.end()) {
        msg = "sql run env not init";
        result = nullptr;
        return execSucces;
    }

    DaoUtil *dbUtil = iterUtil->data();
    if (dbUtil != nullptr) {
        QString errorStr;
        result = dbUtil->execSql(databaseName, sql, execSucces, errorStr);
    }

    return execSucces;
}

// 同步执行
bool DaoClient::execBatchSync(const QString &sql, const QMap<QString, QVariant> &filterDatas, const QMap<QString, QVariant> &data, DaoResultListPtr &result, QString &msg, const QString &databaseName)
{
    QMutexLocker locker(&m_mutex);
    bool execSucces = false;
    auto iterUtil = m_dbConnectToUtil.find(databaseName);
    if (iterUtil == m_dbConnectToUtil.end()) {
        msg = "sql run env not init";
        result = nullptr;
        return execSucces;
    }

    DaoUtil *dbUtil = iterUtil->data();
    if (dbUtil != nullptr) {
        QString errorStr;
        result = dbUtil->execSql(databaseName, sql, filterDatas, data, execSucces, errorStr);
    }

    return execSucces;
}



