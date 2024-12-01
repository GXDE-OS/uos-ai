#include "osinfo.h"
#include "functionhandler.h"

#include <QDebug>

#include <DSysInfo>

DCORE_USE_NAMESPACE

OsInfo::OsInfo(QObject *parent)
    : QObject{parent}
{
    m_pureEnvironment = QProcessEnvironment::systemEnvironment();

    qInfo() << "{"
            << DSysInfo::uosEditionName() << ","
            << DSysInfo::productTypeString() << ","
            << DSysInfo::productVersion()
            << " [ major=" << DSysInfo::majorVersion()
            << ", minor=" << DSysInfo::minorVersion()
            << " ]"
            << "}";
    qInfo() << "cache system environment" << m_pureEnvironment.toStringList();
}

bool OsInfo::isLingLong() const
{
    //TODO:
    //   Now only Deepin Community have linglong ISO
    //UOS don't have linglong edtion,maybe need change
    //the conditon in future.
    bool isLingLong = (DSysInfo::ProductType::Deepin == DSysInfo::productType())
                      && DSysInfo::productVersion() == QString("23");
    return isLingLong;
}

QProcessEnvironment OsInfo::pureEnvironment()
{
    return m_pureEnvironment;
}
