#include "servercodetranslation.h"
#include "networkdefs.h"

#include <QCoreApplication>

QString ServerCodeTranslation::serverCodeTranslation(int code, const QString &message)
{
    QString msg;
    switch (code) {
    case AIServer::TimeoutError:
    case AIServer::NetworkError:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Connection exception, please try again later.");
        break;
    case AIServer::AuthenticationRequiredError:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Connection failed, please check the fill in information.");
        break;
    case AIServer::SenSitiveInfoError:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Sorry, according to relevant laws, regulations and policies, the results are not displayed for the time being.");
        break;
    case AIServer::OperationCanceledError:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Operation canceled.");
        break;
    case AIServer::FREEACCOUNTEXPIRED:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Your free account has expired, please configure your model account to continue using it.");
        break;
    case AIServer::FREEACCOUNTUSAGELIMIT:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Your free account quota has been exhausted, please configure your model account to continue using it.");
        break;
    case AIServer::AudioInputDeviceInvalid:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Invalid input device");
        break;
    case AIServer::AudioOutputDeviceInvalid:
        msg = QCoreApplication::translate("ServerCodeTranslation", "Invalid output device");
        break;
    default:
        break;
    }

    if (msg.isEmpty())
        return message;

    return msg;
}
