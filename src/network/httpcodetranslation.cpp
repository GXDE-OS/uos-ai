#include "httpcodetranslation.h"

using namespace uos_ai;

HttpCodeTranslation::HttpCodeTranslation(QObject *parent) : QObject(parent)
{

}

QString HttpCodeTranslation::translation(QNetworkReply::NetworkError error, const QString &orgMsg)
{
    QString msg;
    switch (error) {
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::SslHandshakeFailedError:
        msg = tr( "Unable to connect to the server, please check your network or try again later.");
        break;
    case QNetworkReply::TimeoutError:
        msg = tr( "Request timeout due to server load or network issues. Please try again later.");
        break;
    case QNetworkReply::AuthenticationRequiredError:
        msg = tr( "Connection failed, please check the fill in information.");
        break;
    case QNetworkReply::OperationCanceledError:
        msg = tr( "Operation canceled.");
        break;
    default:
        break;
    }

    if (msg.isEmpty())
        return orgMsg;

    return msg;
}
