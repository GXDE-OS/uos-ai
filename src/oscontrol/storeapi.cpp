#include "storeapi.h"
#include "osinfo.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QLoggingCategory>
#include <QDBusPendingCall>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)
using namespace uos_ai;

const QString StoreAPI::APPSTORE_CLIENT_SERVICE = "com.home.appstore.client";
const QString StoreAPI::APPSTORE_CLIENT_PATH = "/com/home/appstore/client";
const QString StoreAPI::APPSTORE_CLIENT_INTERFACE = "com.home.appstore.client";
inline constexpr int kStoreAppBriefInfoTimeoutMs = 10000;

StoreAPI::StoreAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_requestHeader = getStoreRequestHeader();
}

void StoreAPI::setRequestHeader(const RequestHeader &header)
{
    m_requestHeader = header;
}

RequestHeader StoreAPI::getRequestHeader() const
{
    return m_requestHeader;
}

void StoreAPI::setServerUrl(const QString &url)
{
    m_serverUrl = url;
}

bool StoreAPI::openBusinessUri(const QString &businessUri)
{
    const QString normalizedBusinessUri = businessUri.trimmed();
    if (normalizedBusinessUri.isEmpty()) {
        qCWarning(logOsControl) << "Failed to open app store: business uri is empty";
        return false;
    }

    const QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qCWarning(logOsControl) << "Failed to open app store: session bus is not connected";
        return false;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        APPSTORE_CLIENT_SERVICE,
        APPSTORE_CLIENT_PATH,
        APPSTORE_CLIENT_INTERFACE,
        "openBusinessUri"
    );
    msg << normalizedBusinessUri;

    connection.asyncCall(msg);
    qCDebug(logOsControl) << "Requested app store business uri:" << normalizedBusinessUri;
    return true;
}

bool StoreAPI::openTargetInAppStore(const QString &target)
{
    const QString businessUri = resolveBusinessUri(target);
    if (businessUri.isEmpty()) {
        qCWarning(logOsControl) << "Failed to resolve app store target:" << target;
        return false;
    }

    return openBusinessUri(businessUri);
}

void StoreAPI::searchApps(const QString &keyword, int page, int maxResults)
{
    QString param = QString("req_page_num=%1&max_results=%2").arg(page).arg(maxResults);
    QUrl url(QString("%1/store-dist-operaton/getTabDetail/searchApp/%2?%3").arg(m_serverUrl).arg(keyword).arg(param));

    QNetworkRequest request(url);

    initRequestHeader(request);

    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });

    qCDebug(logOsControl) << "Search URL:" << url.toString();
}

void StoreAPI::getAppBriefInfo(const QString &packageName)
{
    const QString trimmedPackageName = packageName.trimmed();
    if (trimmedPackageName.isEmpty()) {
        emit appBriefInfoFinished(false, QStringLiteral("package name is empty"), {});
        return;
    }

    const QString urlString = QStringLiteral("%1/store-dist-app/getAppBriefInfo?pkg_name_install_modes=%2:1")
                                  .arg(m_serverUrl, trimmedPackageName);
    QUrl url(urlString);
    QNetworkRequest request(url);

    initRequestHeader(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_networkManager->get(request);
    auto *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [reply]() {
        if (!reply) {
            return;
        }

        reply->setProperty("storeapi_timeout", true);
        reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer]() {
        timeoutTimer->stop();
        timeoutTimer->deleteLater();
        onAppBriefInfoReply(reply);
    });
    timeoutTimer->start(kStoreAppBriefInfoTimeoutMs);

    qCDebug(logOsControl) << "App brief info URL:" << url.toString();
}

void StoreAPI::initRequestHeader(QNetworkRequest &request)
{
    request.setRawHeader("arch", m_requestHeader.arch.toUtf8());
    request.setRawHeader("iswayland", m_requestHeader.iswayland.toUtf8());
    request.setRawHeader("mode", m_requestHeader.mode.toUtf8());
    request.setRawHeader("platform", m_requestHeader.platform.toUtf8());
    request.setRawHeader("region", m_requestHeader.region.toUtf8());
    request.setRawHeader("language", m_requestHeader.language.toUtf8());
    request.setRawHeader("baseline_version", m_requestHeader.baseline_version.toUtf8());
    request.setRawHeader("baseline_main_version", m_requestHeader.baseline_main_version.toUtf8());
    request.setRawHeader("os_build", m_requestHeader.os_build.toUtf8());
    request.setRawHeader("client_version", m_requestHeader.client_version.toUtf8());
    request.setRawHeader("eabi", m_requestHeader.eabi.toUtf8());
    request.setRawHeader("mac", m_requestHeader.mac.toUtf8());
    request.setRawHeader("userid", m_requestHeader.userid.toUtf8());
    request.setRawHeader("referer", m_requestHeader.referer.toUtf8());
    request.setRawHeader("motherboard", m_requestHeader.motherboard.toUtf8());
    request.setRawHeader("cpu_clip", m_requestHeader.cpu_clip.toUtf8());
    request.setRawHeader("is_system_active", m_requestHeader.is_system_active.toUtf8());
    request.setRawHeader("token", m_requestHeader.token.toUtf8());
    request.setRawHeader("supfeatures", m_requestHeader.supFeatures.toUtf8());
    request.setRawHeader("templateId", m_requestHeader.templateId.toUtf8());

    // 设置User-Agent
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     m_requestHeader.user_agent.toUserAgentStr().toUtf8());
}

void StoreAPI::onSearchReply(QNetworkReply *reply)
{
    QList<SearchResult> results;
    QString error;
    bool success = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        qCDebug(logOsControl) << "Response data:" << data;

        success = parseSearchResult(data, results);
        if (!success) {
            error = "Failed to parse search result";
        }
    } else {
        error = reply->errorString();
        qCDebug(logOsControl) << "Network error:" << error;
    }

    emit searchFinished(success, error, results);

    reply->deleteLater();
}

void StoreAPI::onAppBriefInfoReply(QNetworkReply *reply)
{
    QJsonObject appInfo;
    QString error;
    bool success = false;

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        success = parseAppBriefInfo(data, appInfo);
        if (!success) {
            error = QStringLiteral("Failed to parse app brief info");
            qCDebug(logOsControl) << "Failed to parse app brief info response:" << data;
        }
    } else if (reply->property("storeapi_timeout").toBool()) {
        error = QStringLiteral("App brief info request timeout");
        qCWarning(logOsControl) << error;
    } else {
        error = reply->errorString();
        qCWarning(logOsControl) << "App brief info request error:" << error;
    }

    emit appBriefInfoFinished(success, error, appInfo);

    reply->deleteLater();
}

bool StoreAPI::parseSearchResult(const QByteArray &data, QList<SearchResult> &results)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(logOsControl) << "JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qCDebug(logOsControl) << "JSON is not an object";
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (rootObj.contains("code")) {
        int code = rootObj.value("code").toInt();
        if (code != 200) {
            QString message = rootObj.value("message").toString();
            qCDebug(logOsControl) << "API error:" << code << message;
            return false;
        }
    }

    // 解析数据 - datas数组中包含CardDetailInfo对象
    if (rootObj.contains("datas") && rootObj.value("datas").isArray()) {
        QJsonArray datasArray = rootObj.value("datas").toArray();

        for (const QJsonValue &item : datasArray) {
            QJsonObject cardObj = item.toObject();

            // 检查是否包含card_def
            if (cardObj.contains("card_def")) {
                QJsonObject cardDefObj = cardObj.value("card_def").toObject();
                int cardType = cardDefObj.value("card_type").toInt();

                // card_type=11 表示应用列表卡片
                if (cardType == 11 && cardObj.contains("datas")) {
                    QJsonArray appArray = cardObj.value("datas").toArray();

                    for (const QJsonValue &appItem : appArray) {
                        QJsonObject appObj = appItem.toObject();

                        SearchResult result;

                        result.appName = appObj.value("name").toString();
                        result.packageName = appObj.value("package_name").toString();
                        result.version = appObj.value("package_version").toString();
                        result.icon = appObj.value("icon_url").toString();
                        result.description = appObj.value("brief_info").toString();
                        result.score = appObj.value("score").toDouble();
                        result.score = appObj.value("score").toString().toDouble();
                        result.downloadCount = appObj.value("downCount").toInt();
                        result.category = appObj.value("category_name").toString();
                        result.appid = appObj.value("appid").toInt();

                        results.append(result);
                    }
                }
            }
        }
    }

    qCDebug(logOsControl) << "Parsed" << results.size() << "search results";
    return true;
}

bool StoreAPI::parseAppBriefInfo(const QByteArray &data, QJsonObject &appInfo)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logOsControl) << "JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qCWarning(logOsControl) << "App brief info JSON is not an object";
        return false;
    }

    const QJsonObject rootObj = doc.object();
    if (rootObj.contains("code")) {
        const int code = rootObj.value("code").toInt();
        if (code != 200) {
            const QString message = rootObj.value("message").toString();
            qCWarning(logOsControl) << "App brief info API error:" << code << message;
            return false;
        }
    }

    if (rootObj.contains("datas") && rootObj.value("datas").isArray()) {
        const QJsonArray datasArray = rootObj.value("datas").toArray();
        for (const QJsonValue &item : datasArray) {
            if (!item.isObject()) {
                continue;
            }

            const QJsonObject candidate = item.toObject();
            if (!candidate.contains("package_version")) {
                continue;
            }

            appInfo = candidate;
            return true;
        }
    }

    qCWarning(logOsControl) << "App brief info response does not contain package_version";
    return false;
}

bool StoreAPI::openInAppStore(const QString &appid)
{
    return openBusinessUri(QString("app/%1").arg(appid.trimmed()));
}

RequestHeader StoreAPI::getStoreRequestHeader() const
{
    RequestHeader header;

    // 填充系统信息
    header.arch = UosInfo()->systemArch();
    header.iswayland = UosInfo()->isWayland() ? "1" : "0";
    header.mode = UosInfo()->systemMode();
    header.platform = UosInfo()->systemPlatform();
    header.region = UosInfo()->systemRegion();
    header.language = UosInfo()->systemLanguage();
    header.baseline_main_version = UosInfo()->systemMajorVersion();
    header.baseline_version = UosInfo()->systemMinorVersion();
    header.os_build = UosInfo()->systemOsBuild();

    // 填充设备信息
    header.mac = UosInfo()->getMachineId();
    header.motherboard = UosInfo()->getMotherboard();
    header.cpu_clip = UosInfo()->getCpuInfo();
    header.macAddress = UosInfo()->internetMacAddress();

    // 填充User-Agent信息
    header.user_agent.os = header.platform;
    header.user_agent.os_version = header.baseline_main_version + "," + header.baseline_version;
    header.user_agent.os_build = header.os_build;
    header.user_agent.cpuid = UosInfo()->getCpuId();
    header.user_agent.uuid = UosInfo()->getUuid();
    header.user_agent.sn = UosInfo()->getHardDriveSN();

    // 填充特性支持
    header.supFeatures = UosInfo()->getSupFeatures();

    return header;
}

QString StoreAPI::resolveBusinessUri(const QString &target) const
{
    const QString normalizedTarget = target.trimmed();
    if (normalizedTarget.isEmpty()) {
        return QString();
    }

    if (normalizedTarget.startsWith(QLatin1String("tab/")) ||
        normalizedTarget.startsWith(QLatin1String("app/")) ||
        normalizedTarget.startsWith(QLatin1String("app_detail_info/"))) {
        return normalizedTarget;
    }

    return QString("app_detail_info/%1").arg(normalizedTarget);
}
