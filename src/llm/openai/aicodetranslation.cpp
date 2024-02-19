#include "aicodetranslation.h"
#include "servercodetranslation.h"

#include <QCoreApplication>

QString AiCodeTranslation::serverCodeTranlation(int code, const QString &message)
{
    if (message.contains("Rate limit reached")) {
        QString translate = QCoreApplication::translate("AiCodeTranslation", "There are currently too many visitors, please try again later.");
        return translate + QString("(%1)").arg(message);
    }

    return ServerCodeTranslation::serverCodeTranslation(code, message);
}
