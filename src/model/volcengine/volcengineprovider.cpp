#include "volcengineprovider.h"
#include "openai/oaichatmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "builtinprovider.h"

#include <QDebug>

using namespace uos_ai;

VolcengineProvider::VolcengineProvider(QObject *parent)
    : ModelProvider(parent)
{
}

VolcengineProvider::~VolcengineProvider()
{
}

AbstractModel* VolcengineProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "VolcengineProvider: Invalid account";
        return nullptr;
    }
    QVariantHash parameters;
    parameters.insert(STR_KEY_ATTACH_REASONING, true);

    OaiChatModel *model = new OaiChatModel();
    model->setParameters(parameters);
    model->setAccount(acc);
    model->setApiHost(host());

    return model;
}

QString VolcengineProvider::host()
{
    return "https://ark.cn-beijing.volces.com/api/v3";
}
