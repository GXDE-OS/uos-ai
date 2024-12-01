#include "wxqfcodetranslation.h"

#include <QCoreApplication>
#include <QMap>

QMap<int, QString> WXQFCodeTranslation::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {1,      QCoreApplication::translate("WXQFCodeTranslation", "Server internal error")},
        {2,      QCoreApplication::translate("WXQFCodeTranslation", "Service is temporarily unavailable")},
        {3,      QCoreApplication::translate("WXQFCodeTranslation", "The API being called does not exist. Please check the request URL and try again. Generally, there are non-English characters in the URL, such as \"-\". You can enter it manually and try again.")},
        {4,      QCoreApplication::translate("WXQFCodeTranslation", "Model Invocation Error: The model request limit has been reached, or the model is offline. Suggestions: Re-add the model, or contact the customer service of the large model service provider to resolve this issue.")},
        {6,      QCoreApplication::translate("WXQFCodeTranslation", "No interface calling permission, the relevant Qianfan interface is not checked when creating the application")},
        {13,     QCoreApplication::translate("WXQFCodeTranslation", "Failed to obtain token")},
        {14,     QCoreApplication::translate("WXQFCodeTranslation", "IAM authentication failed")},
        {15,     QCoreApplication::translate("WXQFCodeTranslation", "The application does not exist or failed to be created")},
        {17,     QCoreApplication::translate("WXQFCodeTranslation", "The number of requests per day exceeds the limit")},
        {18,     QCoreApplication::translate("WXQFCodeTranslation", "Model Invocation Error: The model request limit has been reached, or the model is offline. Suggestions: Re-add the model, or contact the customer service of the large model service provider to resolve this issue.")},
        {19,     QCoreApplication::translate("WXQFCodeTranslation", "The total number of requests exceeds the limit")},
        {100,    QCoreApplication::translate("WXQFCodeTranslation", "Invalid access_token parameter")},
        {110,    QCoreApplication::translate("WXQFCodeTranslation", "access_token is invalid")},
        {111,    QCoreApplication::translate("WXQFCodeTranslation", "access token expires")},
        {200,    QCoreApplication::translate("WXQFCodeTranslation", "Service error, template does not exist")},
        {1000,   QCoreApplication::translate("WXQFCodeTranslation", "A system error occurred, please try again later")},
        {1001,   QCoreApplication::translate("WXQFCodeTranslation", "The knowledge base server cannot be accessed")},
        {1002,   QCoreApplication::translate("WXQFCodeTranslation", "Knowledge base data download exception")},
        {1004,   QCoreApplication::translate("WXQFCodeTranslation", "Prompt word is too long")},
        {10001,  QCoreApplication::translate("WXQFCodeTranslation", "Model does not exist")},
        {216100,  QCoreApplication::translate("WXQFCodeTranslation", "Request parameter error")},
        {216203,  QCoreApplication::translate("WXQFCodeTranslation", "Image processing failed")},
        {216400,  QCoreApplication::translate("WXQFCodeTranslation", "Service handling exception error")},
        {336000, QCoreApplication::translate("WXQFCodeTranslation", "Service internal error")},
        {336001, QCoreApplication::translate("WXQFCodeTranslation", "The input parameter format is incorrect, such as missing necessary parameters.")},
        {336002, QCoreApplication::translate("WXQFCodeTranslation", "The input parameter body is not in standard JSON format")},
        {336003, QCoreApplication::translate("WXQFCodeTranslation", "Parameter verification is illegal")},
        {336004, QCoreApplication::translate("WXQFCodeTranslation", "Permission control error")},
        {336005, QCoreApplication::translate("WXQFCodeTranslation", "Customized model service apiname does not exist")},
        {336100, QCoreApplication::translate("WXQFCodeTranslation", "Service internal error, please try again later")},
        {336101, QCoreApplication::translate("WXQFCodeTranslation", "Illegal HTTP Method, currently only supports POST requests")},
        {336103, QCoreApplication::translate("WXQFCodeTranslation", "The requested content exceeds the large model internal limit")},
        {336300, QCoreApplication::translate("WXQFCodeTranslation", "internal error")},
        {336301, QCoreApplication::translate("WXQFCodeTranslation", "Vincent diagram model service timeout")},
        {336302, QCoreApplication::translate("WXQFCodeTranslation", "There are security issues with prompt and negative_prompt")},
        {336303, QCoreApplication::translate("WXQFCodeTranslation", "There are security issues with output images")},
        {500000, QCoreApplication::translate("WXQFCodeTranslation", "Server internal error")},
        {500001, QCoreApplication::translate("WXQFCodeTranslation", "Parameter error")},
        {500002, QCoreApplication::translate("WXQFCodeTranslation", "No access")},
    };

    return errorMessages;
}

QString WXQFCodeTranslation::serverCodeTranlation(int code, const QString &message)
{
    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return message;
}
