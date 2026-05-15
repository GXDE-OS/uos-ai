#include "minimaxchatmodel.h"
#include "minimax/minimaxmessageprotocol.h"
#include "global_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

MiniMaxChatModel::MiniMaxChatModel(QObject *parent) : OaiChatModel(parent)
{

}

MiniMaxChatModel::~MiniMaxChatModel()
{

}

QVariantHash MiniMaxChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    if (m_protocol.isNull())
        m_protocol.reset(new MiniMaxMessageProtocol);

    return OaiChatModel::chatCompletion(messages, modelParams);
}
