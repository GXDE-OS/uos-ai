#include "ttscodetranslation.h"
#include "servercodetranslation.h"

#include <QCoreApplication>
#include <QMap>

QMap<int, QString> TtsCodeTranslation::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {10005, QCoreApplication::translate("TtsCodeTranslation", "appid authorization failed")},
        {10006, QCoreApplication::translate("TtsCodeTranslation", "Failed to obtain a certain parameter")},
        {10007, QCoreApplication::translate("TtsCodeTranslation", "Parameter value is illegal")},
        {10010, QCoreApplication::translate("TtsCodeTranslation", "Insufficient engine authorization")},
        {10109, QCoreApplication::translate("TtsCodeTranslation", "Request text length is illegal")},
        {10019, QCoreApplication::translate("TtsCodeTranslation", "session timeout")},
        {10101, QCoreApplication::translate("TtsCodeTranslation", "Engine session ended")},
        {10313, QCoreApplication::translate("TtsCodeTranslation", "appid cannot be empty")},
        {10317, QCoreApplication::translate("TtsCodeTranslation", "Illegal version")},
        {11200, QCoreApplication::translate("TtsCodeTranslation", "permission denied")},
        {11201, QCoreApplication::translate("TtsCodeTranslation", "Daily flow control exceeds limit")},
        {10160, QCoreApplication::translate("TtsCodeTranslation", "The request data format is illegal")},
        {10161, QCoreApplication::translate("TtsCodeTranslation", "base64 decoding failed")},
        {10163, QCoreApplication::translate("TtsCodeTranslation", "Required parameters are missing or the parameters are illegal")},
        {10200, QCoreApplication::translate("TtsCodeTranslation", "Reading data timeout")},
        {10222, QCoreApplication::translate("TtsCodeTranslation", "network anomaly")}
    };

    return errorMessages;
}

QString TtsCodeTranslation::serverCodeTranslation(int code, const QString &message)
{
    if (message.contains("Unauthorized")) {
        return QCoreApplication::translate("TtsCodeTranslation", "Connection failed, please check the fill in information.");
    }

    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return ServerCodeTranslation::serverCodeTranslation(code, message);
}
