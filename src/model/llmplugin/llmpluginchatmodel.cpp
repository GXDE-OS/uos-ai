#include "llmpluginchatmodel.h"
#include "llmmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

LLMPluginChatModel::LLMPluginChatModel(QObject *parent)
    : AbstractChatModel(parent)
{
}

LLMPluginChatModel::~LLMPluginChatModel()
{
    delete m_llmModel;
    m_llmModel = nullptr;
}

void LLMPluginChatModel::setLLMModel(LLMModel *model)
{
    m_llmModel = model;
}

LLMModel* LLMPluginChatModel::llmModel() const
{
    return m_llmModel;
}

QVariantHash LLMPluginChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    m_error.clear();
    if (!m_llmModel) {
        qWarning() << "LLMModel not set";
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_ERROR_MESSAGE] = "LLMModel not set";
        return QVariantHash();
    }

    QJsonObject obj = m_llmModel->generate(buildMessages(messages), modelParams, &LLMPluginChatModel::streamCallback, this);

    QVariantHash result;
    if (obj[GENERATE_RESPONSE_CODE].toInt() != 0) {
        qCWarning(logModel) << "Chat completion failed with error code:"
                               << obj[GENERATE_RESPONSE_CODE].toInt()
                               << "message:" << obj[GENERATE_RESPONSE_ERRORMSG].toString();
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_ERROR_MESSAGE] = obj[GENERATE_RESPONSE_ERRORMSG].toString();
        return result;
    }

    result[STR_KEY_CONTENT] = obj[GENERATE_RESPONSE_CONTENT].toString();

    if (obj.contains(GENERATE_RESPONSE_REFERENCES)) {
        //result[STR_KEY_REFERENCES] = obj[GENERATE_RESPONSE_REFERENCES];
    }

    return result;
}

void LLMPluginChatModel::cancel()
{
    m_abortFlag = true;
    if (m_llmModel) {
        m_llmModel->setAbort();
    }
}

bool LLMPluginChatModel::streamCallback(const QString &deltaData, void *user)
{
    LLMPluginChatModel *self = static_cast<LLMPluginChatModel *>(user);
    if (!deltaData.isEmpty() && !self->m_abortFlag) {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(deltaData.toUtf8(), &parseError);

        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QVariantHash content;
            content[STR_KEY_CONTENT] = jsonObj;

            MetaMessageList msgs = MetaMessage::fromHash(content);
            if (!msgs.isEmpty()) {
                emit self->messageReceived(msgs);
            }
        } else {
            QVariantHash content;
            content[STR_KEY_CONTENT] = deltaData;

            MetaMessageList msgs = MetaMessage::fromHash(content);
            if (!msgs.isEmpty()) {
                emit self->messageReceived(msgs);
            }
        }
    }

    return !self->m_abortFlag;
}

QString LLMPluginChatModel::buildMessages(const QList<ModelMessage> &messages)
{
    QJsonArray ret;
    for (const ModelMessage &msg : messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        QString content;
        for (const MetaMessage &metaMsg : msg.content) {
            if (metaMsg.type == CntText) {
                content += metaMsg.data.toString();
            }
        }
        msgObj["content"] = content;
        ret.append(msgObj);
    }

    return QString::fromUtf8(QJsonDocument(ret).toJson(QJsonDocument::Compact));
}
