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
    bool isLingLong() const;

    QProcessEnvironment pureEnvironment();

private:
    QProcessEnvironment m_pureEnvironment;
};

#define UosInfo() (ESingleton<OsInfo>::getInstance())

#endif // OSINFO_H
