#ifndef OSINFO_H
#define OSINFO_H

#include <QObject>
#include <QProcessEnvironment>
#include <QString>
namespace uos_ai {
enum SupFeature : int {
    Unknown = 0b0,
    SupportSpecificDeb = 0b1,
    SupportSpecificLinglong = 0b10,
    SupportEduCloudPlatform = 0b100,
    SupportDetailPageCards = 0b1000,
    SupportLinglong = 0b10000,
    SupportNewLinglong = 0b100000
};

class OsInfo : public QObject
{
    Q_OBJECT
    explicit OsInfo(QObject *parent = nullptr);
public:
    static OsInfo *instance();
    QProcessEnvironment pureEnvironment();
    void printInfo();

    // 获取系统信息相关方法
    QString systemArch();
    bool isWayland();
    QString systemMode();
    QString systemPlatform();
    QString systemRegion();
    QString systemLanguage();
    QString systemMajorVersion();
    QString systemMinorVersion();
    QString systemOsBuild();
    QString getMachineId();
    QString internetMacAddress();
    QString getMotherboard();
    QString getCpuInfo();
    QString getCpuId();
    QString getUuid();
    QString getHardDriveSN();
    QString getSupFeatures();

private:
    QProcessEnvironment m_pureEnvironment;
};

} // namespace uos_ai

#define UosInfo() OsInfo::instance()

#endif // OSINFO_H
