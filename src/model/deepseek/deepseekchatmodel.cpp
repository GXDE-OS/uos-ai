#include "deepseekchatmodel.h"
#include "deepseek/deepseekmessageprotocol.h"
#include "global_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

DeepSeekChatModel::DeepSeekChatModel(QObject *parent) : OaiChatModel(parent)
{

}

DeepSeekChatModel::~DeepSeekChatModel()
{

}

QVariantHash DeepSeekChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    // 开启思考功能且开启回传 reasoning 功能
    if (modelParams.value(STR_KEY_THINKING, false).toBool() && m_parameters.value(STR_KEY_ATTACH_REASONING, false).toBool()) {
        if (m_protocol.isNull())
            m_protocol.reset(new DeepSeekMessageProtocol);
    } else {
        // 未开启思考功能，移除回传参数
        m_parameters.remove(STR_KEY_ATTACH_REASONING);
    }

    return OaiChatModel::chatCompletion(messages, modelParams);
}
