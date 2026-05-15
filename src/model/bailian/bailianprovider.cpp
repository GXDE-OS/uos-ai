#include "bailianprovider.h"
#include "abstractmodel.h"
#include "bailianchatmodel.h"
#include "global_key_define.h"

#include <QDebug>

namespace uos_ai {

BailianProvider::BailianProvider(QObject *parent)
    : ModelProvider(parent)
{

}

BailianProvider::~BailianProvider()
{

}

AbstractModel* BailianProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    switch (acc->model.arch) {
    case ModelArch::MaLanguage: {
        auto *model = new BailianChatModel();
        model->setAccount(acc);
        model->setApiHost(host());
        return model;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

QString BailianProvider::host()
{
    return QString("https://dashscope.aliyuncs.com/compatible-mode/v1");
}

}
