#include "bigmodelprovider.h"
#include "openai/oaichatmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "builtinprovider.h"

#include <QDebug>

namespace uos_ai {

BigModelProvider::BigModelProvider(QObject *parent)
    : ModelProvider(parent)
{
}

BigModelProvider::~BigModelProvider()
{
}

AbstractModel* BigModelProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "BigModelProvider: Invalid account";
        return nullptr;
    }

    // GLM 都需回传
    QVariantHash parameters;
    parameters.insert(STR_KEY_ATTACH_REASONING, true);

    OaiChatModel *model = new OaiChatModel();
    model->setAccount(acc);
    model->setApiHost(host());
    model->setParameters(parameters);

    return model;
}

QString BigModelProvider::host()
{
    return "https://open.bigmodel.cn/api/paas/v4";
}

} // namespace uos_ai
