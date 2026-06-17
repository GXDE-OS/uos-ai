#include "minimaxprovider.h"
#include "minimaxchatmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "builtinprovider.h"

#include <QDebug>

namespace uos_ai {

MiniMaxProvider::MiniMaxProvider(QObject *parent)
    : ModelProvider(parent)
{
}

MiniMaxProvider::~MiniMaxProvider()
{
}

AbstractModel* MiniMaxProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "MiniMaxProvider: Invalid account";
        return nullptr;
    }

    // 开启回传 reasoning 功能
    QVariantHash parameters;
    parameters.insert(STR_KEY_ATTACH_REASONING, true);

    MiniMaxChatModel *model = new MiniMaxChatModel();

    model->setParameters(parameters);
    model->setAccount(acc);
    model->setApiHost(host());
    
    return model;
}

QString MiniMaxProvider::host()
{
    return "https://api.minimaxi.com/v1";
}

} // namespace uos_ai
