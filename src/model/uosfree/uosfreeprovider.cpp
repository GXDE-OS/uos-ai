#include "uosfreeprovider.h"
#include "deepseek/deepseekchatmodel.h"
#include "onlinesearchmodel.h"
#include "abstractmodel.h"
#include "global_key_define.h"
#include "tas/uosaccountencoder.h"
#include "builtinprovider.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

namespace uos_ai {

UosFreeProvider::UosFreeProvider(QObject *parent)
    : ModelProvider(parent)
{

}

UosFreeProvider::~UosFreeProvider()
{

}

AbstractModel* UosFreeProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qCWarning(logModel) << "Invalid model account provided";
        return nullptr;
    }

    auto decrypted = decryptAccount(acc);

    QVariantHash parameters;
    // 自动选择模型或没有配置模型时，默认使用deepseek 3.2
    if (decrypted->model.id.compare(UOS_FREE_MODEL_AUTO, Qt::CaseInsensitive) == 0 || decrypted->model.modelId.isEmpty()) {
        decrypted->model = BuiltinProvider::instance()->getModelInfo(UOS_FREE_DEEPSEEK_V3_2);
    }

    parameters.insert(STR_KEY_ATTACH_REASONING, true);

    switch (decrypted->model.arch) {
    case ModelArch::MaLanguage: {
        if (decrypted->model.id.compare(UOS_FREE_ONLINE_SEARCH, Qt::CaseInsensitive) == 0) {
            auto model = new OnlineSearchModel();
            model->setAccount(decrypted);
            return model;
        } else {
            auto model = new OaiChatModel();
            model->setAccount(decrypted);
            model->setApiHost(host());
            model->setParameters(parameters);
            return model;
        }
        break;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

QString UosFreeProvider::host()
{
    return QString("https://ark.cn-beijing.volces.com/api/v3");
}

ModelAccountPtr UosFreeProvider::decryptAccount(const ModelAccountPtr &acc)
{
    ModelAccountPtr ret = acc;
    QString apikey = ret->account.auth.value(STR_KEY_API_KEY).toString();

    UosAccountEncoder encoder;
    QVariantHash auth;
    auth[STR_KEY_API_KEY] = std::get<0>(encoder.decrypt(apikey));

    ret->account.auth = auth;
    return ret;
}

} // namespace uos_ai
