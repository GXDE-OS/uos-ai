#ifndef UOSFREEACCOUNTS_H
#define UOSFREEACCOUNTS_H

#include "network/httpclient.h"

#include <QNetworkReply>
#include <QString>

/* ModelType {
    USER = 0,           // 用户自己的账号
    FREE_NORMAL = 1,    // 普通免费账号
    FREE_KOL = 2,       // KOL免费账号
    LOCAL = 3          // 本地模型
};

llm {
    UOS_FREE = 82,
}
*/

namespace uos_ai {

struct UosFreeAccountActivity {
    QString buttonNameChina;  // 按钮名称中文
    QString buttonNameEnglish; // 按钮名称英文
    QString url; // 跳转的url地址
    int display; // 是否显示 0:隐藏，1:显示
    QString type; // 类型，根据类型判断哪个页面
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

struct UosFreeModelActivity {
    int type; // 模型类型：deepseek 81, 百度千帆 20
    bool inActivityPeriod; // 账号是否免费发放活动期间 true：该大模型免费账号发放中；false：该大模型免费账号活动结束
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

struct UosFreeAccount {
    int type; // 账号类型:普通账号 general，KOL账号 KOL
    QString appid; // 账号
    QString appkey; // 账号
    QString appsecret; // 账号
    int useLimit; // 最大使用限制
    bool hasUsed; // 已经使用次数
    int llmModel; // 大模型类型
    QString modelUrl; // 大模型接口地址
    QDateTime expTime; // 失效时间
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

class UosFreeAccounts
{
public:
    static UosFreeAccounts &instance();

    ~UosFreeAccounts();

    // 按钮显示增加接口
    QNetworkReply::NetworkError freeAccountButtonDisplay(const QString &type, UosFreeAccountActivity &freeAccountActivity);

    // 获取免费账号接口
    QNetworkReply::NetworkError getFreeAccount(int type, int llm, UosFreeAccount &freeAccount, int &status);

    // 判断账号是否可用
    QNetworkReply::NetworkError getDeterAccountLegal(const QString &appkey, int &available, QString &modelUrl, bool &claimAgain, QString *msg = nullptr);

    QNetworkReply::NetworkError claimAccountUsage(const QString &appkey, int &result, QString &msg);

    QNetworkReply::NetworkError checkFreeModelActivity(int llm, int &result, UosFreeModelActivity &freeModelActivity);

    // 增加账号使用次数
    QNetworkReply::NetworkError increaseUse(const QString &appkey, int chatAction);

private:
    UosFreeAccounts();

    void initServerAddress();

    QPair<QNetworkReply::NetworkError, QJsonObject> getHttpResponse(const HttpClient::HttpResponse &response);

private:
    QString m_serverAddress;
};

}

#endif // UOSFREEACCOUNTS_H
