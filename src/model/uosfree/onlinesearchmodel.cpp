#include "onlinesearchmodel.h"
#include "global_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

OnlineSearchModel::OnlineSearchModel(QObject *parent) : OaiChatModel(parent)
{
    m_host = QString("https://ark.cn-beijing.volces.com/api/v3/bots");
}

OnlineSearchModel::~OnlineSearchModel()
{

}

void OnlineSearchModel::setApiHost(const QString &host)
{
    qCWarning(logModel) << "unable to set api host." << host;
}

QVariantHash OnlineSearchModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    m_account->model.modelId = modelParams.value(STR_KEY_THINKING, false).toBool() ? searchThinkingId() : searchId();
    return OaiChatModel::chatCompletion(messages, modelParams);
}

