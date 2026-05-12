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

    OaiChatModel *model = new OaiChatModel();
    model->setAccount(acc);
    model->setApiHost(host());

    return model;
}

QString VolcengineProvider::host()
{
    return "https://ark.cn-beijing.volces.com/api/v3";
}
