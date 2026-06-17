#ifndef STOREAPI_H
#define STOREAPI_H

#include "osinfo.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QDBusMessage>
#include <QDBusConnection>

namespace uos_ai {

// 请求头参数结构体
struct RequestHeader {
    // 系统信息
    QString arch = "amd64";                    // 系统架构
    QString iswayland = "0";                   // 是否使用Wayland
    QString mode = "desktop";                  // 系统模式
    QString platform = "Professional";         // 系统平台
    QString region = "CN";                     // 地区
    QString language = "zh_CN";                // 语言
    QString baseline_version = "20";           // 系统基线版本
    QString baseline_main_version = "20";      // 系统大版本
    QString os_build = "2023.10.01";           // 系统OsBuild
    QString client_version = "5.6.3.1";        // 客户端版本
    QString eabi = "";                         // 显卡能力（保留）

    // 设备信息
    QString mac = "default-machine-id";           // 机器ID
    QString motherboard = "default-mainboard";    // 主板名称
    QString cpu_clip = "default CPU";            // CPU名称
    QString macAddress = "00:00:00:00:00:00"; // 网卡地址

    // 用户信息
    QString userid = "";                       // 用户ID
    QString token = "";                        // 用户token
    QString is_system_active = "1";            // 系统是否激活
    QString referer = "";                      // 来源页面

    // 特性支持
    QString supFeatures = "1111";              // 支持的特性
    QString templateId = "template123";        // 模板ID

    // User-Agent数据
    struct UserAgentData {
        QString device_id = "default-device-id";
        QString client = "app_store";
        QString client_version = "5.6.3.1";
        QString os = "Professional";
        QString os_version = "20,20";
        QString os_build = "2023.10.01";
        QString channel = "default";
        QString youthSys = "false";
        QString cpuid = "default-cpuid";
        QString uuid = "default-uuid";
        QString sn = "default-sn";

        QString toUserAgentStr() {
            QString data;
            data.append(QString("device_id/%1").arg(device_id));
            data.append(" ");
            data.append(QString("client/%1").arg(client));
            data.append(" ");
            data.append(QString("client_version/%1").arg(client_version));
            data.append(" ");
            data.append(QString("os/%1").arg(os));
            data.append(" ");
            data.append(QString("os_version/%1").arg(os_version));
            data.append(" ");
            data.append(QString("os_build/%1").arg(os_build));
            data.append(" ");
            data.append(QString("channel/%1").arg(channel));
            data.append(" ");
            data.append(QString("cpuid/%1").arg(cpuid));
            data.append(" ");
            data.append(QString("uuid/%1").arg(uuid));
            data.append(" ");
            data.append(QString("SN/%1").arg(sn));
            return data;
        }
    } user_agent;
};

struct SearchResult {
    QString appName;
    QString packageName;
    QString version;
    QString icon;
    QString description;
    double score;
    int downloadCount;
    QString category;
    int appid;
};

class StoreAPI : public QObject
{
    Q_OBJECT

public:
    explicit StoreAPI(QObject *parent = nullptr);

    void setRequestHeader(const RequestHeader &header);
    RequestHeader getRequestHeader() const;
    void searchApps(const QString &keyword, int page = 1, int maxResults = 3);
    void getAppBriefInfo(const QString &packageName);
    void setServerUrl(const QString &url);
    bool openBusinessUri(const QString &businessUri);
    bool openTargetInAppStore(const QString &target);
    bool openInAppStore(const QString &appid);

signals:
    void searchFinished(bool success, const QString &error, const QList<SearchResult> &results);
    void appBriefInfoFinished(bool success, const QString &error, const QJsonObject &appInfo);

private slots:
    void onSearchReply(QNetworkReply *reply);
    void onAppBriefInfoReply(QNetworkReply *reply);

private:
    void initRequestHeader(QNetworkRequest &request);
    bool parseSearchResult(const QByteArray &data, QList<SearchResult> &results);
    bool parseAppBriefInfo(const QByteArray &data, QJsonObject &appInfo);
    RequestHeader getStoreRequestHeader() const;
    QString resolveBusinessUri(const QString &target) const;

    QNetworkAccessManager *m_networkManager;
    RequestHeader m_requestHeader;
    QString m_serverUrl = "https://appstore.uniontech.com";

    static const QString APPSTORE_CLIENT_SERVICE;
    static const QString APPSTORE_CLIENT_PATH;
    static const QString APPSTORE_CLIENT_INTERFACE;
};

}
#endif // STOREAPI_H
