#include "modelhub/modelhubprovider.h"
#include "modelhub/modelhubchatmodel.h"
#include "global_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

namespace uos_ai {

const QString model_1_5 = "YouRong-1.5B";
const QString model_7 = "YouRong-7B";

ModelHubProvider::ModelHubProvider(QObject *parent)
    : ModelProvider(parent)
{

}

ModelHubProvider::~ModelHubProvider()
{

}

QList<ModelAccountPtr> ModelHubProvider::modelList()
{
    static const QString idPrefix = "modelhub_";

    QList<ModelAccountPtr> list;

    auto models = ModelhubWrapper::installedModels();
    QWriteLocker lk(&mtx);

    for (auto key : wrapper.keys()) {
        if (!models.contains(key))
            wrapper.remove(key);
    }

    for (const QString &model : models) {
        if (model.isEmpty() || model == "BAAI-bge-large-zh-v1.5")
            continue;

        ModelAccountPtr acc(new ModelAccount);
        acc->id = idPrefix + model;
        acc->account.provider = STR_KEY_MODELHUB;
        acc->account.id = STR_KEY_MODELHUB;
        acc->account.name = STR_KEY_MODELHUB;
        acc->model.id = model;
        acc->model.arch = MaLanguage;
        acc->model.ability = ModelAbilities(MaText);
        acc->network = STR_KEY_LOCAL;

        if (model == model_1_5) {
            acc->model.name = tr("YouRong 1.5B");
            acc->model.ability = ModelAbilities(MaText);
        } else if (model == model_7) {
            acc->model.name = tr("YouRong 7B");
            acc->model.ability = ModelAbilities(MaText);
        } else {
            acc->model.name = model;
        }

        list << acc;
        if (!wrapper.contains(model)) {
            qCInfo(logModel) << "Found local model" << model;
            auto ins = new ModelhubWrapper(model);
            ins->setKeepLive(true);
            wrapper.insert(model, QSharedPointer<ModelhubWrapper>(ins));
        }
    }

    return list;
}

AbstractModel* ModelHubProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    QVariantHash params;
    params.insert(STR_KEY_TIMEOUT, 10 * 60 * 1000);

    switch (acc->model.arch) {
    case ModelArch::MaLanguage: {
        auto *model = new ModelHubChatModel(getWrapper(acc->model.id));
        model->setAccount(acc);
        model->setEnableTool(acc->model.id == model_1_5 || acc->model.id == model_7);
        model->setParameters(params);
        return model;
    }
    default:
        qWarning() << "Unsupported model architecture:" << acc->model.arch;
        break;
    }

    return nullptr;
}

QSharedPointer<ModelhubWrapper> ModelHubProvider::getWrapper(const QString &id)
{
    QReadLocker lk(&mtx);
    return wrapper.value(id);
}

} // namespace uos_ai
