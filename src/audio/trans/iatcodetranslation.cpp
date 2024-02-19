#include "iatcodetranslation.h"
#include "servercodetranslation.h"

#include <QCoreApplication>
#include <QMap>

QMap<int, QString> IatCodeTranslation::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {10005, QCoreApplication::translate("IatCodeTranslation", "appid authorization failed")},
        {10006, QCoreApplication::translate("IatCodeTranslation", "Failed to obtain a certain parameter")},
        {10007, QCoreApplication::translate("IatCodeTranslation", "Parameter value is illegal")},
        {10010, QCoreApplication::translate("IatCodeTranslation", "Insufficient engine authorization")},
        {10014, QCoreApplication::translate("IatCodeTranslation", "Session timeout")},
        {10019, QCoreApplication::translate("IatCodeTranslation", "Session timeout")},
        {10043, QCoreApplication::translate("IatCodeTranslation", "Audio decoding failed")},
        {10101, QCoreApplication::translate("IatCodeTranslation", "Engine session ended")},
        {10114, QCoreApplication::translate("IatCodeTranslation", "Session timeout")},
        {10139, QCoreApplication::translate("IatCodeTranslation", "Parameter error")},
        {10313, QCoreApplication::translate("IatCodeTranslation", "appid cannot be empty")},
        {10317, QCoreApplication::translate("IatCodeTranslation", "Illegal version")},
        {11200, QCoreApplication::translate("IatCodeTranslation", "permission denied")},
        {11201, QCoreApplication::translate("IatCodeTranslation", "Daily flow control exceeds limit")},
        {10160, QCoreApplication::translate("IatCodeTranslation", "The request data format is illegal")},
        {10161, QCoreApplication::translate("IatCodeTranslation", "base64 decoding failed")},
        {10163, QCoreApplication::translate("IatCodeTranslation", "Required parameters are missing or the parameters are illegal")},
        {10165, QCoreApplication::translate("IatCodeTranslation", "Invalid handle")},
        {10200, QCoreApplication::translate("IatCodeTranslation", "Reading data timeout")}
    };

    return errorMessages;
}

QString IatCodeTranslation::serverCodeTranslation(int code, const QString &message)
{
    if (message.contains("Unauthorized")) {
        return QCoreApplication::translate("IatCodeTranslation", "Connection failed, please check the fill in information.");
    }

    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return ServerCodeTranslation::serverCodeTranslation(code, message);
}
