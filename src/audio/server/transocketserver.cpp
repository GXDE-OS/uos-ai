#include "transocketserver.h"
#include "networkdefs.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QCryptographicHash>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

QString getCurrentRFC1123Timestamp()
{
    const QString weekdayName[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    const QString monthName[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    QDateTime dateTime = QDateTime::currentDateTimeUtc();
    QDate date = dateTime.date();
    QTime time = dateTime.time();

    int weekday = date.dayOfWeek() - 1;

    QString day = QString("%1").arg(date.day(), 2, 10, QChar('0'));
    QString hour = QString("%1").arg(time.hour(), 2, 10, QChar('0'));
    QString minute = QString("%1").arg(time.minute(), 2, 10, QChar('0'));
    QString second = QString("%1").arg(time.second(), 2, 10, QChar('0'));

    return QString("%1, %2 %3 %4 %5:%6:%7 GMT")
           .arg(weekdayName[weekday])
           .arg(day)
           .arg(monthName[date.month()])
           .arg(date.year())
           .arg(hour)
           .arg(minute)
           .arg(second);
}

Q_DECLARE_LOGGING_CATEGORY(logAudio)

TranSocketServer::TranSocketServer(const AccountProxy &account, QObject *parent)
    : TranServer(parent)
    , m_account(account)
{
    m_account.socketProxy.socketProxyType = SocketProxyType::NO_PROXY;
    m_manager = new QNetworkAccessManager(this);
}

void TranSocketServer::sendText(const QString &text)
{
    qCDebug(logAudio) << "Sending text for translation, length:" << text.length();
    QString m_text = text;

    QByteArray body;
    QJsonObject postdata;
    QString content; // 用于存储最终的Base64编码字符串

    // 将QString转换为QByteArray，使用UTF-8编码
    QByteArray textData = m_text.toUtf8();

    // 使用QBase64进行Base64编码
    QByteArray base64Encoded = textData.toBase64();
    // 将QByteArray转换回QString，注意编码
    content = QString(base64Encoded);
    postdata["common"] = QJsonObject{{"app_id", m_account.appId}};
    postdata["business"] = QJsonObject{{"from", "cn"}, {"to", "en"}};
    postdata["data"] = QJsonObject{{"text", content}};
    QJsonDocument jsonDoc(postdata);
    body = jsonDoc.toJson(QJsonDocument::Compact);

    QString method = "POST";
    QString path = "/v2/its";
    m_businessArgs = {{"from", "cn"}, {"to", "en"}};
    QString m_date = getCurrentRFC1123Timestamp();
    QString m_host = "itrans.xfyun.cn";
    QString url = "https://" + m_host + "/v2/its";
    
    qCDebug(logAudio) << "Preparing request to URL:" << url;
    
    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Method", method.toUtf8());
    request.setRawHeader("Host", m_host.toUtf8());
    request.setRawHeader("Date", m_date.toUtf8());

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(body);

    // 获取哈希结果的QByteArray
    QByteArray hashResult = hash.result();
    QString digest = "SHA-256=" + hashResult.toBase64();
    request.setRawHeader("Digest", digest.toUtf8());

    QString signatureOrigin = "host: " + m_host + "\n";
    signatureOrigin += "date: " + m_date + "\n";
    signatureOrigin += method + " " + path + " HTTP/1.1" + "\n";;
    signatureOrigin += "digest: " + digest;

    QByteArray signature = QMessageAuthenticationCode::hash(signatureOrigin.toUtf8(), m_account.apiSecret.toUtf8(), QCryptographicHash::Sha256).toBase64();


    QString authorizationOrigin = QString("api_key=\"%1\", algorithm=\"hmac-sha256\", headers=\"host date request-line digest\", signature=\"%2\"")
                                  .arg(m_account.apiKey)
                                  .arg(QString(signature));

    request.setRawHeader("Authorization",authorizationOrigin.toUtf8());
    QByteArray base64Text = text.toUtf8();

    qCDebug(logAudio) << "Sending translation request";
    QNetworkReply *reply =  m_manager->post(request, body);
    QObject::connect(reply, &QNetworkReply::readyRead, this, [=](){
        const QByteArray &data = reply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        QString trans_result = obj.value("data").toObject().value("result").toObject().value("trans_result").toObject().value("dst").toString();
        qCDebug(logAudio) << "Received translation result, length:" << trans_result.length();
        emit textReceived(trans_result);
    });
}
