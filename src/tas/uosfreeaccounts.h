#ifndef UOSFREEACCOUNTS_H
#define UOSFREEACCOUNTS_H
#include "tasdef.h"
#include "serverdefs.h"
#include "networkdefs.h"

#include <QString>
#include <tuple>

class HttpAccessmanager;
class HttpEventLoop;

class UosFreeAccounts
{
public:
    static UosFreeAccounts &instance();

    ~UosFreeAccounts();

    // 按钮显示增加接口
    QNetworkReply::NetworkError freeAccountButtonDisplay(const QString &type, UosFreeAccountActivity &freeAccountActivity);

    // 获取免费账号接口
    QNetworkReply::NetworkError getFreeAccount(const ModelType type, UosFreeAccount &freeAccount, int &status);

    // 判断账号是否可用
    QNetworkReply::NetworkError getDeterAccountLegal(const QString &appkey, int &available);

    // 增加账号使用次数
    QNetworkReply::NetworkError increaseUse(const QString &appkey);

    QString getLastError() const;

    int getErrorCode() const;

private:
    UosFreeAccounts();

    QPair<QNetworkReply::NetworkError, QJsonObject> getHttpResponse(const QSharedPointer<HttpAccessmanager> hacc, const HttpEventLoop *loop);

private:
    QString m_lastError;

    unsigned int m_status;
};

#endif // UOSFREEACCOUNTS_H
