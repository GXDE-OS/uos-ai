#include "bailianchatmodel.h"
#include "bailianmessageprotocol.h"
#include "global_key_define.h"

using namespace uos_ai;

BailianChatModel::BailianChatModel(QObject *parent) : OaiChatModel(parent)
{

}

BailianChatModel::~BailianChatModel()
{

}

QVariantHash BailianChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    if (m_protocol.isNull())
        m_protocol.reset(new BailianMessageProtocol);

    return OaiChatModel::chatCompletion(messages, modelParams);
}
