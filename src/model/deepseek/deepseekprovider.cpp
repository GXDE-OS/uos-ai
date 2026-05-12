#include "deepseekprovider.h"
#include "deepseekchatmodel.h"
#include "modelinfo.h"
#include "global_key_define.h"
#include "builtinprovider.h"

#include <QDebug>

namespace uos_ai {

DeepSeekProvider::DeepSeekProvider(QObject *parent)
    : ModelProvider(parent)
{
}

DeepSeekProvider::~DeepSeekProvider()
{
}

AbstractModel* DeepSeekProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "DeepSeekProvider: Invalid account";
        return nullptr;
    }

    // 创建DeepSeek聊天模型实例
    DeepSeekChatModel *model = new DeepSeekChatModel();

    QVariantHash parameters;
    // 只有deepseek 3.2模型，开启回传 reasoning 功能
    static QSet<QString> reasoningBack = {DEEPSEEK_DEEPSEEK_V3_2, DEEPSEEK_DEEPSEEK_V4_FLASH, DEEPSEEK_DEEPSEEK_V4_PRO};
    if (reasoningBack.contains(acc->model.id)) {
        parameters.insert(STR_KEY_ATTACH_REASONING, true);
    }
    model->setParameters(parameters);

    // 设置模型账户信息
    model->setAccount(acc);
    model->setApiHost(host());
    
    return model;
}

QString DeepSeekProvider::host()
{
    // DeepSeek官方API地址
    return "https://api.deepseek.com";
}

} // namespace uos_ai
