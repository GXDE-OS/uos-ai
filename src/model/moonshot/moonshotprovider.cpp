#include "moonshotprovider.h"
#include "openai/oaichatmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "builtinprovider.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

namespace uos_ai {

MoonshotProvider::MoonshotProvider(QObject *parent)
    : ModelProvider(parent)
{
}

MoonshotProvider::~MoonshotProvider()
{
}

AbstractModel* MoonshotProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qCWarning(logModel) << "MoonshotProvider: Invalid account";
        return nullptr;
    }

    OaiChatModel *model = new OaiChatModel();
    model->setAccount(acc);
    model->setApiHost(host());

    return model;
}

QString MoonshotProvider::host()
{
    return "https://api.moonshot.cn/v1";
}

} // namespace uos_ai
