#include "360codetranslation.h"

#include <QCoreApplication>
#include <QDebug>

QMap<int, QString> CodeTranslation360::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {1004, QCoreApplication::translate("CodeTranslation360", "Authentication failed, insufficient balance")},
        {1006, QCoreApplication::translate("CodeTranslation360", "Authentication failed, daily limit exceeded")},
        {1005, QCoreApplication::translate("CodeTranslation360", "There are currently too many visitors, please try again later.")},
        {1003, QCoreApplication::translate("CodeTranslation360", "The service is abnormal. Please contact the development team for troubleshooting.")}
    };

    return errorMessages;
}

QString CodeTranslation360::serverCodeTranlation(int code, const QString &message)
{
    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return message;
}

