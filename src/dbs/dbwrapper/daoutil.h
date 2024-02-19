#ifndef DAOUTIL_H
#define DAOUTIL_H

#include <QObject>
#include <QVariant>
#include <QMap>
#include <QList>
#include <QSqlDatabase>
#include <QSharedPointer>

typedef QMap<QString, QVariant> QVariantString;
typedef QList<QVariantString> DaoResultList;
typedef QSharedPointer<DaoResultList> DaoResultListPtr;

class DaoUtilHelper;

class DaoUtil : public QObject
{
    Q_OBJECT
public:
    DaoUtil(QObject *parent = nullptr);

    DaoUtil(bool encrypt, QObject *parent = nullptr);

    ~DaoUtil();

    // 初始化数据库
    void initDb(const QString &dbDir, const QString &dbPath, const QString &user, const QString &passwd, const QString &driverType, const QString &dbDriveName);

    // 执行完整或者拼接好的SQL语句
    DaoResultListPtr execSql(const QString &dbUnique, const QString &sql, bool &isSuccess, QString &errorStr);

    // 执行批处理，带事务的操作
    DaoResultListPtr execSql(const QString &dbUnique, const QString &sql, const QMap<QString, QVariant> &filterDatas, const QMap<QString, QVariant> &data, bool &isSuccess, QString &errorStr);

private:
    bool keepOpenning();

    inline QSqlQuery makeSqlQuery(QSqlDatabase &dataBase);

    inline void clearSqlQuery(QSqlDatabase &dataBase);

private: signals:
    void sigExecResult(QString, DaoResultListPtr, bool, QString);
    void sigExecResultEx(QString, DaoResultListPtr, bool, QString);

private:
    DaoUtilHelper *m_helper = nullptr;

    bool m_keepOpenning;

    bool m_doEncrypted;

};

#endif // DAOUTIL_H
