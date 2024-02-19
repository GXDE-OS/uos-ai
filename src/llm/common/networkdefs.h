#ifndef NETWORKDEFS_H
#define NETWORKDEFS_H

#include <QAbstractSocket>
#include <QNetworkReply>

namespace AIServer {
enum ErrorType {
    NoError = 0,

    // network layer errors [relating to the destination server] (1-99):
    ConnectionRefusedError = 1,
    RemoteHostClosedError,
    HostNotFoundError,
    TimeoutError,
    OperationCanceledError,
    SslHandshakeFailedError,
    TemporaryNetworkFailureError,
    NetworkSessionFailedError,
    BackgroundRequestNotAllowedError,
    TooManyRedirectsError,
    InsecureRedirectError,
    UnknownNetworkError = 99,

    // proxy errors (101-199):
    ProxyConnectionRefusedError = 101,
    ProxyConnectionClosedError,
    ProxyNotFoundError,
    ProxyTimeoutError,
    ProxyAuthenticationRequiredError,
    UnknownProxyError = 199,

    // content errors (201-299):
    ContentAccessDenied = 201,
    ContentOperationNotPermittedError,
    ContentNotFoundError,
    AuthenticationRequiredError,
    ContentReSendError,
    ContentConflictError,
    ContentGoneError,
    UnknownContentError = 299,

    // protocol errors
    ProtocolUnknownError = 301,
    ProtocolInvalidOperationError,
    ProtocolFailure = 399,

    // Server side errors (401-499)
    InternalServerError = 401,
    OperationNotImplementedError,
    ServiceUnavailableError,
    UnknownServerError = 499,

    SocketAccessError = 600,
    SocketResourceError,
    SocketTimeoutError,
    DatagramTooLargeError,
    NetworkError,
    AddressInUseError,
    SocketAddressNotAvailableError,
    UnsupportedSocketOperationError,
    UnfinishedSocketOperationError,
    ProxyConnectionTimeoutError,
    ProxyProtocolError,
    OperationError,
    SslInternalError,
    SslInvalidUserDataError,
    TemporaryError = 699,

    AudioInputDeviceInvalid = 800,
    AudioOutputDeviceInvalid = 801,

    // 敏感词错误
    SenSitiveInfoError = 8000,
    ServerRateLimitError = 8001,
    // 免费账号异常
    FREEACCOUNTEXPIRED = 9000,
    FREEACCOUNTUSAGELIMIT = 9001,
    AccountInvalid = 9002
};

inline AIServer::ErrorType networkReplyErrorToAiServerError(QNetworkReply::NetworkError type)
{
    static const QMap<QNetworkReply::NetworkError, AIServer::ErrorType> errorMap = {
        {QNetworkReply::NoError, AIServer::NoError},
        {QNetworkReply::ConnectionRefusedError, AIServer::ConnectionRefusedError},
        {QNetworkReply::RemoteHostClosedError, AIServer::RemoteHostClosedError},
        {QNetworkReply::HostNotFoundError, AIServer::HostNotFoundError},
        {QNetworkReply::TimeoutError, AIServer::TimeoutError},
        {QNetworkReply::OperationCanceledError, AIServer::OperationCanceledError},
        {QNetworkReply::SslHandshakeFailedError, AIServer::SslHandshakeFailedError},
        {QNetworkReply::TemporaryNetworkFailureError, AIServer::TemporaryNetworkFailureError},
        {QNetworkReply::NetworkSessionFailedError, AIServer::NetworkSessionFailedError},
        {QNetworkReply::BackgroundRequestNotAllowedError, AIServer::BackgroundRequestNotAllowedError},
        {QNetworkReply::TooManyRedirectsError, AIServer::TooManyRedirectsError},
        {QNetworkReply::InsecureRedirectError, AIServer::InsecureRedirectError},
        {QNetworkReply::ProxyConnectionRefusedError, AIServer::ProxyConnectionRefusedError},
        {QNetworkReply::ProxyConnectionClosedError, AIServer::ProxyConnectionClosedError},
        {QNetworkReply::ProxyNotFoundError, AIServer::ProxyNotFoundError},
        {QNetworkReply::ProxyTimeoutError, AIServer::ProxyTimeoutError},
        {QNetworkReply::ProxyAuthenticationRequiredError, AIServer::ProxyAuthenticationRequiredError},
        {QNetworkReply::ContentAccessDenied, AIServer::ContentAccessDenied},
        {QNetworkReply::ContentOperationNotPermittedError, AIServer::ContentOperationNotPermittedError},
        {QNetworkReply::ContentNotFoundError, AIServer::ContentNotFoundError},
        {QNetworkReply::AuthenticationRequiredError, AIServer::AuthenticationRequiredError},
        {QNetworkReply::ContentReSendError, AIServer::ContentReSendError},
        {QNetworkReply::ContentConflictError, AIServer::ContentConflictError},
        {QNetworkReply::ContentGoneError, AIServer::ContentGoneError},
        {QNetworkReply::InternalServerError, AIServer::InternalServerError},
        {QNetworkReply::OperationNotImplementedError, AIServer::OperationNotImplementedError},
        {QNetworkReply::ServiceUnavailableError, AIServer::ServiceUnavailableError},
        {QNetworkReply::ProtocolUnknownError, AIServer::ProtocolUnknownError},
        {QNetworkReply::ProtocolInvalidOperationError, AIServer::ProtocolInvalidOperationError},
        {QNetworkReply::UnknownProxyError, AIServer::UnknownProxyError},
        {QNetworkReply::UnknownContentError, AIServer::UnknownContentError},
        {QNetworkReply::ProtocolFailure, AIServer::ProtocolFailure},
        {QNetworkReply::UnknownServerError, AIServer::UnknownServerError}
    };

    return errorMap.value(type, AIServer::UnknownNetworkError);
}

inline AIServer::ErrorType socketErrorToAiServerError(QAbstractSocket::SocketError type)
{
    static const QMap<int, AIServer::ErrorType> errorMap = {
        {-1, AIServer::NoError},
        {QAbstractSocket::ConnectionRefusedError, AIServer::ConnectionRefusedError},
        {QAbstractSocket::RemoteHostClosedError, AIServer::RemoteHostClosedError},
        {QAbstractSocket::HostNotFoundError, AIServer::HostNotFoundError},
        {QAbstractSocket::SocketAccessError, AIServer::SocketAccessError},
        {QAbstractSocket::SocketResourceError, AIServer::SocketResourceError},
        {QAbstractSocket::SocketTimeoutError, AIServer::TimeoutError},
        {QAbstractSocket::DatagramTooLargeError, AIServer::DatagramTooLargeError},
        {QAbstractSocket::NetworkError, AIServer::NetworkError},
        {QAbstractSocket::AddressInUseError, AIServer::AddressInUseError},
        {QAbstractSocket::SocketAddressNotAvailableError, AIServer::SocketAddressNotAvailableError},
        {QAbstractSocket::UnsupportedSocketOperationError, AIServer::UnsupportedSocketOperationError},
        {QAbstractSocket::ProxyAuthenticationRequiredError, AIServer::ProxyAuthenticationRequiredError},
        {QAbstractSocket::SslHandshakeFailedError, AIServer::SslHandshakeFailedError},
        {QAbstractSocket::UnfinishedSocketOperationError, AIServer::UnfinishedSocketOperationError},
        {QAbstractSocket::ProxyConnectionRefusedError, AIServer::ProxyConnectionRefusedError},
        {QAbstractSocket::ProxyConnectionClosedError, AIServer::ProxyConnectionClosedError},
        {QAbstractSocket::ProxyConnectionTimeoutError, AIServer::ProxyConnectionTimeoutError},
        {QAbstractSocket::ProxyNotFoundError, AIServer::ProxyNotFoundError},
        {QAbstractSocket::ProxyProtocolError, AIServer::ProxyProtocolError},
        {QAbstractSocket::OperationError, AIServer::OperationError},
        {QAbstractSocket::SslInternalError, AIServer::SslInternalError},
        {QAbstractSocket::SslInvalidUserDataError, AIServer::SslInvalidUserDataError},
        {QAbstractSocket::TemporaryError, AIServer::TemporaryError}
    };

    return errorMap.value(type, AIServer::UnknownNetworkError);
}
}
#endif // NETWORKDEFS_H
