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
    return OaiChatModel::chatCompletion(messages, modelParams);
}
