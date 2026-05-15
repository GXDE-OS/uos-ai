#include "osinfo.h"

#include <QLoggingCategory>
#include <DSysInfo>
#include <QProcess>
#include <QFile>
#include <QSettings>
#include <QNetworkInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLocale>

DCORE_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

OsInfo::OsInfo(QObject *parent)
    : QObject{parent}
{
    qCDebug(logOsControl) << "Initializing OsInfo with system environment";
    m_pureEnvironment = QProcessEnvironment::systemEnvironment();
}

OsInfo *OsInfo::instance()
{
    static OsInfo ins;
    return &ins;
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

QString OsInfo::systemArch()
{
    QString arch = "";
    QProcess process;
    process.start("dpkg", {"--print-architecture"});
    process.waitForFinished(-1);
    if (process.exitCode() == 0 && process.exitStatus() == QProcess::NormalExit) {
        QString outData = process.readAllStandardOutput();
        if (!outData.isEmpty()) {
            arch = outData.trimmed();
        }
    }
    return arch;
}

bool OsInfo::isWayland()
{
    // 检查XDG_SESSION_TYPE环境变量
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    return sessionType.toLower() == "wayland";
}

QString OsInfo::systemMode()
{
    // 默认返回desktop
    return "desktop";
}

QString OsInfo::systemPlatform()
{
    QString platform = DSysInfo::uosEditionName(QLocale(QLocale::English));
    switch (DSysInfo::uosEditionType()) {
    case DSysInfo::UosProfessional:
        platform = "Professional";
        break;
    case DSysInfo::UosHome:
        platform = "Home";
        break;
    case DSysInfo::UosCommunity:
        platform = "Community";
        break;
    case DSysInfo::UosEducation:
        platform = "Education";
        break;
    case DSysInfo::UosMilitary:
        platform = "Military";
        break;
#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 6, 11, 1) && DTK_VERSION < DTK_VERSION_CHECK(5, 7, 0, 0))
    case DSysInfo::UosPersonal:
        platform = "Personal";
        break;
#endif
    default:
        platform = "Unknown";
        break;
    }

    return platform;
}

QString OsInfo::systemRegion()
{
    // 默认返回CN
    return "CN";
}

QString OsInfo::systemLanguage()
{
    return QLocale::system().name();
}

QString OsInfo::systemMajorVersion()
{
    QFile file("/etc/os-version");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            if (line.startsWith("MajorVersion=")) {
                file.close();
                return line.split("=")[1];
            }
        }
        file.close();
    }
    return "25";
}

QString OsInfo::systemMinorVersion()
{
    QFile file("/etc/os-version");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            if (line.startsWith("MinorVersion=")) {
                file.close();
                return line.split("=")[1];
            }
        }
        file.close();
    }
    return "0";
}

QString OsInfo::systemOsBuild()
{
    QFile file("/etc/os-version");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            if (line.startsWith("OsBuild=")) {
                file.close();
                return line.split("=")[1];
            }
        }
        file.close();
    }
    return "";
}

QString OsInfo::getMachineId()
{
    QFile file("/etc/machine-id");
    if (file.open(QIODevice::ReadOnly)) {
        QString id = file.readAll().trimmed();
        file.close();
        return id;
    }
    return "";
}

QString OsInfo::internetMacAddress()
{
    QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &net : nets) {
        if (net.flags().testFlag(QNetworkInterface::IsUp) &&
            net.flags().testFlag(QNetworkInterface::IsRunning) &&
            !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return net.hardwareAddress();
        }
    }
    return "";
}

QString OsInfo::getMotherboard()
{
    return "Unknown";
}

QString OsInfo::getCpuInfo()
{
    // 从/proc/cpuinfo读取CPU信息
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            if (line.startsWith("model name")) {
                file.close();
                return line.split(":")[1].trimmed();
            }
        }
        file.close();
    }
    return "Unknown CPU";
}

QString OsInfo::getCpuId()
{
    return "unknown";
}

QString OsInfo::getUuid()
{
    return "unknown";
}

QString OsInfo::getHardDriveSN()
{
    return "unknown";
}

QString OsInfo::getSupFeatures()
{
    int supFeatures = SupFeature::SupportSpecificDeb
                      | SupFeature::SupportEduCloudPlatform
                      | SupFeature::SupportDetailPageCards;

    QDBusInterface interface("com.home.appstore.daemon",
                             "/appstore",
                             "com.home.appstore.daemon.interface",
                             QDBusConnection::systemBus());

    if (interface.isValid()) {
        bool supported = interface.property("IsLinglongSupported").toBool();
        if (supported) {
            supFeatures |= SupFeature::SupportSpecificLinglong
                          | SupFeature::SupportLinglong
                          | SupFeature::SupportNewLinglong;
        }
    } else {
        qCWarning(logOsControl) << "Failed to connect to D-Bus interface for IsLinglongSupported";
    }

    return QString::number(supFeatures, 2);
}
