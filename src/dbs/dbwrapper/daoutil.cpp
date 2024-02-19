#include "daoutil.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QDir>
#include <QDateTime>
#include <QMap>
#include <QUuid>
#include <QDebug>

class DaoUtilHelper
{
public:
    QMap<QString, QSqlDatabase> m_dbMap;
};


DaoUtil::DaoUtil(QObject *parent)
    : QObject(parent)
    , m_keepOpenning(false)
    , m_doEncrypted(false)
{
    m_helper = new DaoUtilHelper();
}

DaoUtil::DaoUtil(bool encrypt, QObject *parent)
    : QObject(parent)
    , m_keepOpenning(encrypt)
    , m_doEncrypted(encrypt)
{
    m_helper = new DaoUtilHelper();
}

DaoUtil::~DaoUtil()
{
    if (m_helper) {
        auto dbIter = m_helper->m_dbMap.begin();
        while (dbIter != m_helper->m_dbMap.end()) {
            if (dbIter->isOpen()) {
                dbIter->close();
            }
            dbIter++;
        }
        delete m_helper;
        m_helper = nullptr;
    }
}

void DaoUtil::initDb(const QString &dbDir, const QString &dbPath, const QString &user, const QString &passwd, const QString &driverType, const QString &dbDriveName)
{
    QSqlDatabase dataBase = QSqlDatabase::addDatabase(driverType, dbDriveName);
    QDir dir(dbDir);
    dir.mkpath(dbDir);
    dataBase.setDatabaseName(dbDir + QString("/") + dbPath);

    QSqlQuery query = makeSqlQuery(dataBase);

    // 检测数据库是否有效
    bool flag = query.exec("SELECT * FROM sqlite_master;");

    QString sqlError = query.lastError().text();
    if (!flag && sqlError.contains("database disk image is malformed")) {
        dataBase.close();
        QFile file(dbDir + QString("/") + dbPath);
        bool flag = file.rename(dbDir + QString("/bak_%1").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss")) + dbPath);
        if (flag) {
            dataBase = QSqlDatabase::cloneDatabase(dataBase, dbDir + QString("/") + dbDriveName);
            dataBase.setDatabaseName(dbDir + QString("/") + dbPath);
            dataBase.setUserName(user);
            dataBase.setPassword(passwd);
        }
    }

    clearSqlQuery(dataBase);

    m_helper->m_dbMap.insert(dbDriveName, dataBase);

    return;
}

DaoResultListPtr DaoUtil::execSql(const QString &dbUnique, const QString &sql, bool &isSuccess, QString &errorStr)
{
    DaoResultListPtr listPtr(new DaoResultList());
    isSuccess = false;
    if (m_helper == nullptr) {
        errorStr = "not initDb, please initDb first";
        return listPtr;
    }

    auto iterDb = m_helper->m_dbMap.find(dbUnique);
    if (iterDb == m_helper->m_dbMap.end()) {
        errorStr = "not found sql database unique";
        return listPtr;
    }

    QSqlDatabase &dbBase = iterDb.value();
    QSqlQuery query = makeSqlQuery(dbBase);

    if (query.exec(sql)) {
        while (query.next()) {
            QVariantString rowMap;
            QSqlRecord record = query.record();
            for (int i = 0; i < record.count(); i++) {
                QString filedName = record.fieldName(i);
                QString filedValue = record.value(i).toString();
                rowMap.insert(filedName, filedValue);
            }
            listPtr->append(rowMap);
        }
        isSuccess = true;
    } else {
        errorStr = query.lastError().text();
    }

    clearSqlQuery(dbBase);

    return listPtr;
}

DaoResultListPtr DaoUtil::execSql(const QString &dbUnique, const QString &sql, const QMap<QString, QVariant> &filterDatas, const QMap<QString, QVariant> &data, bool &isSuccess, QString &errorStr)
{
    isSuccess = false;
    DaoResultListPtr listPtr(new DaoResultList());

    if (m_helper == nullptr) {
        errorStr = "not initDb, please initDb first";
        return listPtr;
    }

    auto iterDb = m_helper->m_dbMap.find(dbUnique);
    if (iterDb == m_helper->m_dbMap.end()) {
        errorStr = "not found sql database unique";
        return listPtr;
    }

    if (sql.contains(" WHERE ") && filterDatas.isEmpty()) {
        errorStr = "Parameter error invalid sql command";
        return listPtr;
    }

    QSqlDatabase &dbBase = iterDb.value();

    if (dbBase.driver()->hasFeature(QSqlDriver::Transactions)) {
        QSqlQuery query = makeSqlQuery(dbBase);
        if (dbBase.transaction()) {
            if (!query.prepare(sql)) {
                errorStr = query.lastError().text();
                dbBase.rollback();
                return listPtr;
            }
            if (sql.contains(" WHERE ")) {
                for (const auto &key : filterDatas.keys()) {
                    query.bindValue(":" + key, filterDatas.value(key));
                }
            }
            for (const auto &key : data.keys()) {
                query.bindValue(":" + key, data.value(key));
            }

            if (query.exec()) {
                while (query.next()) {
                    QVariantString rowMap;
                    QSqlRecord record = query.record();
                    for (int i = 0; i < record.count(); i++) {
                        QString filedName = record.fieldName(i);
                        QVariant filedValue = record.value(i);
                        rowMap.insert(filedName, filedValue);
                    }
                    listPtr->append(rowMap);
                }

                if (!dbBase.commit()) {
                    errorStr = dbBase.lastError().text();
                    dbBase.rollback();
                    return listPtr;
                }

                isSuccess = true;
            } else {
                errorStr = query.lastError().text();
                dbBase.rollback();
            }
        } else {
            errorStr = "enter Transactions failed";
        }
        clearSqlQuery(dbBase);
    } else {
        errorStr = "driver don't has Transactions feature";
    }

    return listPtr;
}

bool DaoUtil::keepOpenning()
{
    return 0;
}

inline QSqlQuery DaoUtil::makeSqlQuery(QSqlDatabase &dataBase)
{
    if (!dataBase.isOpen()) {
        if (!dataBase.open()) {
            qWarning() << "open database err: " << dataBase.lastError().text();
            return QSqlQuery();
        }
        auto query = QSqlQuery(dataBase);
        if (m_keepOpenning) {
            if (!query.exec("PRAGMA key='DSQLITECIPHER';")) {
                qWarning() << "open database err: " << dataBase.lastError().text();
                return QSqlQuery();
            }
        }
        return query;
    }
    return QSqlQuery(dataBase);
}

inline void DaoUtil::clearSqlQuery(QSqlDatabase &dataBase)
{
    if (!m_keepOpenning && dataBase.isOpen()) {
        dataBase.close();
    }
}
