#include "xfcodetranslation.h"

#include <QCoreApplication>
#include <QMap>

QMap<int, QString> XFCodeTranslation::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {10000, QCoreApplication::translate("XFCodeTranslation", "Error while upgrading to WebSocket")},
        {10001, QCoreApplication::translate("XFCodeTranslation", "Error reading user's message via WebSocket")},
        {10002, QCoreApplication::translate("XFCodeTranslation", "Error sending message to user via WebSocket")},
        {10003, QCoreApplication::translate("XFCodeTranslation", "The user's message format is incorrect")},
        {10004, QCoreApplication::translate("XFCodeTranslation", "Schema error for user data")},
        {10005, QCoreApplication::translate("XFCodeTranslation", "User parameter value is wrong")},
        {10006, QCoreApplication::translate("XFCodeTranslation", "User concurrency error: The current user is already connected, and the same user cannot connect to multiple places at the same time.")},
        {10007, QCoreApplication::translate("XFCodeTranslation", "User traffic is limited: The service is processing the user's current problem and needs to wait for the processing to be completed before sending a new request. (You must wait for a complete reply from the  model before sending the next question)")},
        {10008, QCoreApplication::translate("XFCodeTranslation", "Insufficient service capacity, please contact staff")},
        {10009, QCoreApplication::translate("XFCodeTranslation", "Failed to establish connection with engine")},
        {10010, QCoreApplication::translate("XFCodeTranslation", "Error receiving engine data")},
        {10011, QCoreApplication::translate("XFCodeTranslation", "Error sending data to engine")},
        {10012, QCoreApplication::translate("XFCodeTranslation", "Engine internal error")},
        {10013, QCoreApplication::translate("XFCodeTranslation", "The input content does not pass the review and is suspected of violating the rules. Please readjust the input content.")},
        {10014, QCoreApplication::translate("XFCodeTranslation", "The output content involves sensitive information, the review fails, and subsequent results cannot be displayed to the user.")},
        {10015, QCoreApplication::translate("XFCodeTranslation", "appid is in the blacklist")},
        {10016, QCoreApplication::translate("XFCodeTranslation", "Appid authorization class error. For example: this function is not activated, the corresponding version is not activated, the token is insufficient, the concurrency exceeds authorization, etc.")},
        {10017, QCoreApplication::translate("XFCodeTranslation", "Clear history failed")},
        {10019, QCoreApplication::translate("XFCodeTranslation", "Indicates that the content of this session has a tendency to involve illegal information; it is recommended that developers give the user a prompt after receiving this error code to enter information that involves illegal information.")},
        {10110, QCoreApplication::translate("XFCodeTranslation", "Service is busy, please try again later")},
        {10163, QCoreApplication::translate("XFCodeTranslation", "The parameters of the request engine are abnormal, and the engine schema check fails.")},
        {10222, QCoreApplication::translate("XFCodeTranslation", "Engine network abnormality")},
        {10907, QCoreApplication::translate("XFCodeTranslation", "The number of tokens exceeds the upper limit. The number of words in the conversation history and question is too large and needs to be simplified.")},
        {11200, QCoreApplication::translate("XFCodeTranslation", "Authorization error: The appId does not have authorization for related functions or the business volume exceeds the limit")},
        {11201, QCoreApplication::translate("XFCodeTranslation", "Authorization error: Daily flow control limit exceeded. Exceeded the maximum number of visits for the day")},
        {11202, QCoreApplication::translate("XFCodeTranslation", "There are currently too many visitors, please try again later.")},
        {11203, QCoreApplication::translate("XFCodeTranslation", "There are currently too many visitors, please try again later.")},
    };

    return errorMessages;
}

QString XFCodeTranslation::serverCodeTranslation(int code, const QString &message)
{
    if (message.contains("Unauthorized")) {
        return QCoreApplication::translate("XFCodeTranslation", "Connection failed, please check the fill in information.");
    }

    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return message;
}
