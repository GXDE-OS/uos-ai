#include "osinfo.h"
#include "functionhandler.h"

#include <QLoggingCategory>
#include <DSysInfo>

DCORE_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

OsInfo::OsInfo(QObject *parent)
    : QObject{parent}
{
    qCDebug(logOsControl) << "Initializing OsInfo with system environment";
    m_pureEnvironment = QProcessEnvironment::systemEnvironment();
}

QProcessEnvironment OsInfo::pureEnvironment()
{
    return m_pureEnvironment;
}

void OsInfo::printInfo()
{
    qCInfo(logOsControl) << "System information: {"
                         << DSysInfo::uosEditionName() << ","
                         << DSysInfo::productTypeString() << ","
                         << DSysInfo::productVersion()
                         << " [ major=" << DSysInfo::majorVersion()
                         << ", minor=" << DSysInfo::minorVersion()
                         << " ]"
                         << "}";
    qCInfo(logOsControl) << "Cached system environment:" << m_pureEnvironment.toStringList();
}
