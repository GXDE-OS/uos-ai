#include "openaicompatibleprovider.h"
#include "abstractmodel.h"
#include "oaichatmodel.h"
#include "global_key_define.h"

#include <QDebug>

namespace uos_ai {

OpenaiCompatibleProvider::OpenaiCompatibleProvider(QObject *parent)
    : ModelProvider(parent)
{

}

OpenaiCompatibleProvider::~OpenaiCompatibleProvider()
{

}

AbstractModel* OpenaiCompatibleProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    switch (acc->model.arch) {
    case ModelArch::MaLanguage: {
        //todo 如果模型id是deepseek，需创建deepseekprotocol，
        //todo 判断模型是deepseek 3.2 则开启reasoning回传
        auto *model = new OaiChatModel();
        model->setAccount(acc);
        model->setApiHost(acc->account.additional.value(STR_KEY_PROVIDER_HOST).toString());
        return model;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

} // namespace uos_ai
