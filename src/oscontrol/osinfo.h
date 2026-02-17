#ifndef OSINFO_H
#define OSINFO_H

#include "esingleton.h"

#include <QObject>
#include <QProcessEnvironment>

class OsInfo : public QObject
{
    Q_OBJECT
    SINGLETONIZE_CLASS(OsInfo);

    explicit OsInfo(QObject *parent = nullptr);
public:
    QProcessEnvironment pureEnvironment();
    void printInfo();

private:
    QProcessEnvironment m_pureEnvironment;
};

#define UosInfo() (ESingleton<OsInfo>::getInstance())

#endif // OSINFO_H
