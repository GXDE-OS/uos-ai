#include "modelvalidator.h"
#include "modelvendor.h"
#include "abstractchatmodel.h"
#include "network/httpclient.h"
#include "global_key_define.h"
#include "conversation/messagenode.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

ModelValidator::ModelValidator(QObject *parent)
    : QObject(parent)
{
}

ModelValidator::~ModelValidator()
{
}

ModelValidator::ModelValidationResult ModelValidator::validate(const ModelAccountPtr &account)
{
    ModelValidationResult result;

    if (account.data() == nullptr || account->model.arch != MaLanguage) {
        result.success = false;
        result.errorMessage = tr("Invalid model account");
        result.errorType = InvalidModel;
        return result;
    }

    return validateChatModel(account);
}

ModelValidator::ModelValidationResult ModelValidator::validateChatModel(const ModelAccountPtr &account)
{
    ModelValidationResult result;

    ModelVendor *vendor = ModelVendor::instance();
    QSharedPointer<AbstractModel> model = vendor->createModel(account);

    if (model.isNull()) {
        result.success = false;
        result.errorMessage = tr("Unsupported model type");
        result.errorType = InvalidModel;
        return result;
    }

    auto chatModel = qSharedPointerCast<AbstractChatModel>(model);
    if (!chatModel) {
        result.success = false;
        result.errorMessage = tr("Model does not support chat completion");
        result.errorType = InvalidModel;
        return result;
    }

    QList<ModelMessage> messages;
    ModelMessage msg;
    msg.role = "user";
    MetaMessage content;
    content.type = CntText;
    content.data = "Hello";
    msg.content.append(content);
    messages.append(msg);

    QVariantHash params;
    params.insert(STR_KEY_STREAM, true);
    bool ok = false;
    connect(chatModel.data(), &AbstractChatModel::messageReceived, this, [chatModel, &ok](const MetaMessageList &msg){
        if (!msg.isEmpty()) {
            ok = true;
            chatModel->cancel();
        }
    });

    QVariantHash response = chatModel->chatCompletion(messages, params);
    if (ok) {
        result.success = true;
        result.errorMessage = tr("Model is available");
        result.errorType = NoError;
    } else {
        result.success = false;
        response = chatModel->lastError();
        if (response.contains(STR_KEY_ERROR)) {
            result.errorType = static_cast<GErrorType>(response.value(STR_KEY_ERROR).toInt());

            if (response.contains(STR_KEY_HTTP_ERROR)) {
                QNetworkReply::NetworkError httpError = static_cast<QNetworkReply::NetworkError>(response.value(STR_KEY_HTTP_ERROR).toInt());
                result.errorMessage = tr("HTTP error: %1").arg(httpError);
            }

            if (response.contains(STR_KEY_ERROR_MESSAGE)) {
                QString errorMsg = response.value(STR_KEY_ERROR_MESSAGE).toString();
                if (!errorMsg.isEmpty()) {
                    result.errorMessage = errorMsg;
                }
            }
        } else {
            result.errorMessage = tr("Unknown error");
            result.errorType = GErrorType::HttpError;
        }
    }

    return result;
}

QString ModelValidator::parseErrorResponse(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        return QString::fromUtf8(data);
    }

    if (!doc.isObject()) {
        return QString::fromUtf8(data);
    }

    QJsonObject obj = doc.object();

    if (obj.contains("error")) {
        QJsonValue errorValue = obj.value("error");
        if (errorValue.isObject()) {
            QJsonObject errorObj = errorValue.toObject();
            if (errorObj.contains("message")) {
                return errorObj.value("message").toString();
            }
            return QJsonDocument(errorObj).toJson(QJsonDocument::Compact);
        } else if (errorValue.isString()) {
            return errorValue.toString();
        }
    }

    if (obj.contains("message")) {
        return obj.value("message").toString();
    }

    return QString::fromUtf8(data);
}
