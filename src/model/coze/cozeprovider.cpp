#include "cozeprovider.h"
#include "abstractmodel.h"
#include "cozeagentchat.h"
#include "global_key_define.h"

#include <QDebug>

namespace uos_ai {

CozeProvider::CozeProvider(QObject *parent)
    : ModelProvider(parent)
{

}

CozeProvider::~CozeProvider()
{

}

AbstractModel* CozeProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    switch (acc->model.arch) {
    case ModelArch::MaLanguage: {
        auto *model = new CozeAgentChat();
        model->setAccount(acc);
        model->setBotId(acc->account.auth.value("botID").toString());
        model->setApiHost(host());
        return model;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

QString CozeProvider::host()
{
    return QString("https://api.coze.cn/v3/chat");
}

} // namespace uos_ai
