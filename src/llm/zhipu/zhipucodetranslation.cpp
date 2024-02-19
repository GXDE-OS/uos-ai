#include "zhipucodetranslation.h"

#include <QCoreApplication>
#include <QMap>

QMap<int, QString> ZhiPuCodeTranslation::errorMessages()
{
    static const QMap<int, QString> errorMessages = {
        {500,  QCoreApplication::translate("ZhiPuCodeTranslation", "Server internal error")},
        {1000, QCoreApplication::translate("ZhiPuCodeTranslation", "Authentication failed")},
        {1001, QCoreApplication::translate("ZhiPuCodeTranslation", "The Authentication parameter was not received in the header and authentication could not be performed.")},
        {1002, QCoreApplication::translate("ZhiPuCodeTranslation", "The Authentication Token is illegal. Please confirm that the Authentication Token is delivered correctly.")},
        {1003, QCoreApplication::translate("ZhiPuCodeTranslation", "Authentication Token has expired, please regenerate/obtain it")},
        {1004, QCoreApplication::translate("ZhiPuCodeTranslation", "Verification via Authentication Token failed")},
        {1100, QCoreApplication::translate("ZhiPuCodeTranslation", "Account reading and writing")},
        {1110, QCoreApplication::translate("ZhiPuCodeTranslation", "Your account is currently inactive. Please check account information")},
        {1111, QCoreApplication::translate("ZhiPuCodeTranslation", "Your account does not exist")},
        {1112, QCoreApplication::translate("ZhiPuCodeTranslation", "Your account has been locked, please contact customer service to unlock it")},
        {1113, QCoreApplication::translate("ZhiPuCodeTranslation", "Your account is in arrears, please recharge and try again")},
        {1120, QCoreApplication::translate("ZhiPuCodeTranslation", "Unable to successfully access your account, please try again later")},
        {1200, QCoreApplication::translate("ZhiPuCodeTranslation", "API call error")},
        {1210, QCoreApplication::translate("ZhiPuCodeTranslation", "API call parameters are incorrect, please check the documentation")},
        {1211, QCoreApplication::translate("ZhiPuCodeTranslation", "Model does not exist, please check the model code")},
        {1212, QCoreApplication::translate("ZhiPuCodeTranslation", "The current model does not support the ${method} calling method")},
        {1213, QCoreApplication::translate("ZhiPuCodeTranslation", "${field} Parameters not received normally")},
        {1214, QCoreApplication::translate("ZhiPuCodeTranslation", "The ${field}  parameter is illegal. Please check the documentation")},
        {1215, QCoreApplication::translate("ZhiPuCodeTranslation", "${field1} and ${field2} cannot be set at the same time, please check the documentation")},
        {1220, QCoreApplication::translate("ZhiPuCodeTranslation", "You do not have access to  ${API_name}")},
        {1221, QCoreApplication::translate("ZhiPuCodeTranslation", "API ${API_name} is offline")},
        {1222, QCoreApplication::translate("ZhiPuCodeTranslation", "API ${API_name}  does not exist")},
        {1230, QCoreApplication::translate("ZhiPuCodeTranslation", "API call process error")},
        {1231, QCoreApplication::translate("ZhiPuCodeTranslation", "You have requested: ${request_id}")},
        {1232, QCoreApplication::translate("ZhiPuCodeTranslation", "When getting asynchronous request results, please use task_id")},
        {1233, QCoreApplication::translate("ZhiPuCodeTranslation", "Task: ${task_id} does not exist")},
        {1234, QCoreApplication::translate("ZhiPuCodeTranslation", "Network error, error id: ${error_id}, please contact customer service")},
        {1235, QCoreApplication::translate("ZhiPuCodeTranslation", "Network error, error id: ${error_id}, please contact customer service")},
        {1260, QCoreApplication::translate("ZhiPuCodeTranslation", "API run error")},
        {1261, QCoreApplication::translate("ZhiPuCodeTranslation", "Prompt super long")},
        {1300, QCoreApplication::translate("ZhiPuCodeTranslation", "API call blocked by policy")},
        {1301, QCoreApplication::translate("ZhiPuCodeTranslation", "The system has detected that the input or generated content may contain unsafe or sensitive content. Please avoid entering prompts that may easily generate sensitive content. Thank you for your cooperation.")},
        {1302, QCoreApplication::translate("ZhiPuCodeTranslation", "There are currently too many visitors, please try again later.")},
        {1303, QCoreApplication::translate("ZhiPuCodeTranslation", "There are currently too many visitors, please try again later.")},
        {1304, QCoreApplication::translate("ZhiPuCodeTranslation", "This API has reached the limit of calls for today. If you need more, please contact customer service to purchase.")},
        {1305, QCoreApplication::translate("ZhiPuCodeTranslation", "There are currently too many visitors, please try again later.")}
    };

    return errorMessages;
}

QString ZhiPuCodeTranslation::serverCodeTranlation(int code, const QString &message)
{
    if (errorMessages().contains(code))
        return errorMessages()[code] + QString("(%1)").arg(message);

    return message;
}
