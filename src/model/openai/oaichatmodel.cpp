#include "oaichatmodel.h"
#include "modelinfo.h"
#include "oaimessageprotocol.h"
#include "global_key_define.h"
#include "network/httpclient.h"
#include "network/httpcodetranslation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

//#define MOCK_CHAT

OaiChatModel::OaiChatModel(QObject *parent) : AbstractChatModel(parent)
{

}

OaiChatModel::~OaiChatModel()
{

}

#if !defined(QT_DEBUG) || !defined(MOCK_CHAT)
QVariantHash OaiChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    m_error.clear();
    if (m_protocol.isNull())
        m_protocol.reset(new OaiMessageProtocol);

    if (m_parameters.value(STR_KEY_ATTACH_REASONING, false).toBool())
        m_protocol->setEnableReasoning(true);

    m_protocol->initMessage(messages);
    QJsonObject requestObj = m_protocol->params(modelParams);

    requestObj[STR_KEY_MODEL] = m_account->model.modelId;
    requestObj[STR_KEY_MESSAGES] = m_protocol->messages();

    HttpClient client;
    client.setTimeOut(m_parameters.value(STR_KEY_TIMEOUT, 10 * 60 * 1000).toInt());

    connect(this, &OaiChatModel::requestCancel, &client, &HttpClient::doAbort);

    if (modelParams.value(STR_KEY_STREAM).toBool())
        connect(&client, &HttpClient::readyRead, this, &OaiChatModel::chunkReceived);

    const QString suffix = "/chat/completions";
    QString url = apiHost();
    if (!url.contains(suffix))
        url.append(suffix);

    QVariantHash header;
    header.insert("Authorization", QString("Bearer %0").arg(m_account->account.auth.value(STR_KEY_API_KEY).toString()));
    HttpClient::HttpResponse response = client.request(url, requestObj, header);

    QVariantHash ret;
    {
        QVariantHash content;
        bool parsed = modelParams.value(STR_KEY_STREAM).toBool()
                      ? m_protocol->parseChunk(response.data, content)
                      : m_protocol->parseResponse(response.data, content);
        if (parsed) {
            for (auto it = content.begin(); it != content.end(); ++it)
                ret.insert(it.key(), it.value());
        }
    }

    if (response.error != QNetworkReply::NoError) {
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = response.error;
        QString message;
        if (!response.data.isEmpty()) {
            QJsonObject errorObj = QJsonDocument::fromJson(response.data).object().value("error").toObject();
            message = errorObj.value("message").toString();
            if (!message.isEmpty())
                message.prepend("\n\n");
        }

        m_error[STR_KEY_ERROR_MESSAGE] = HttpCodeTranslation::translation(response.error, response.errorString) + message;
    }

    return ret;
}
#else
#include <QTimer>
#include <QEventLoop>
QVariantHash OaiChatModel::chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams)
{
    QVariantHash content;
    QString result;
    int count = 0;

    QTimer t;
    t.start(100);
    QEventLoop loop;

    connect(&t, &QTimer::timeout, this, [this, &result, &count, &loop]() {
        count++;
        QString str = " " + QString::number(count);
        if (count % 10 == 0)
            str.append("\n");

        result += str;

        QVariantHash content;
        content[STR_KEY_CONTENT] = str;
        MetaMessageList msgs = MetaMessage::fromHash(content);;
        emit messageReceived(msgs);
        if (count > 600)
            loop.quit();
    });

    connect(this, &OaiChatModel::requestCancel, &loop, [this, &loop](){
        loop.quit();
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = QNetworkReply::OperationCanceledError;
        m_error[STR_KEY_ERROR_MESSAGE] = HttpCodeTranslation::translation(QNetworkReply::OperationCanceledError, "");
    });

    loop.exec();

    content[STR_KEY_CONTENT] = result;
    return content;
}
#endif

void OaiChatModel::setApiHost(const QString &host)
{
    m_host = host;
}

QString OaiChatModel::apiHost() const
{
    return m_host;
}

void OaiChatModel::cancel()
{
    emit requestCancel();
}

void OaiChatModel::chunkReceived(const QByteArray &chunk)
{
    QVariantHash content;
    MetaMessageList msgs;
    if (m_protocol->parseChunk(chunk, content)) {
        // FIXME 流式输出的tool calls是不完整的，这里发送会是错误的数据。
        content.remove(STR_KEY_TOOL_CALLS);

        msgs = MetaMessage::fromHash(content);
    }

    if (!msgs.isEmpty())
        emit messageReceived(msgs);
}
