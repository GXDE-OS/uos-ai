#include "openaiprovider.h"
#include "abstractmodel.h"
#include "oaichatmodel.h"
#include "global_key_define.h"

#include <QDebug>

namespace uos_ai {

OpenaiProvider::OpenaiProvider(QObject *parent)
    : ModelProvider(parent)
{

}

OpenaiProvider::~OpenaiProvider()
{

}

AbstractModel* OpenaiProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    switch (acc->model.arch) {
    case ModelArch::MaLanguage: {
        auto *model = new OaiChatModel();
        model->setAccount(acc);
        // 设置OpenAI官方API地址
        model->setApiHost(host());
        return model;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

QString OpenaiProvider::host()
{
    return QString("https://api.openai.com/v1");
}

} // namespace uos_ai
