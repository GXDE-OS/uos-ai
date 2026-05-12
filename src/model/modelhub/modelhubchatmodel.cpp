#include "modelhub/modelhubchatmodel.h"
#include "modelinfo.h"
#include "openai/oaimessageprotocol.h"
#include "global_key_define.h"
#include "network/httpclient.h"
#include "network/httpcodetranslation.h"
#include "externalllm/modelhubwrapper.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

ModelHubChatModel::ModelHubChatModel(QSharedPointer<ModelhubWrapper> ins, QObject *parent)
    : OaiChatModel(parent)
    , wrapper(ins)
{

}

ModelHubChatModel::~ModelHubChatModel()
{

}

QVariantHash ModelHubChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    m_error.clear();

    if (wrapper.isNull()) {
        qCWarning(logModel) << "Modelhub wrapper is null - account invalid";
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        return {};
    }

    wrapper->ensureRunning();

    setApiHost(wrapper->urlPath(""));

    auto params = modelParams;
    if (!m_toolUse)
        params.remove(STR_KEY_TOOLS);

    return OaiChatModel::chatCompletion(messages, params);
}
