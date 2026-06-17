#include "aiwriter.h"
#include "agent/research/writingorchestrator.h"
#include "agent/research/workspacestore.h"
#include "model/modelvendor.h"
#include "conversation/conversationrecord.h"
#include "global_key_define.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QLocale>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

AIWriter::AIWriter(QObject *parent) : AbstractAssistant(parent)
{
}

AIWriter::~AIWriter()
{
}

QString AIWriter::getRecentDocs(int maxCount)
{
    QList<WritingWorkspace> workspaces = WorkspaceStore::instance()->listAll();

    // Collect all articles across all workspaces
    struct ArticleEntry {
        QString id;
        QString name;
        QDateTime updatedAt;
        QString conversationId;
    };
    QList<ArticleEntry> articles;

    for (const WritingWorkspace &ws : workspaces) {
        for (const QString &artId : ws.articleIds()) {
            const Article *art = ws.article(artId);
            if (!art || !art->isValid() || !art->hasContent())
                continue;
            articles.append({art->id(), art->title(), art->updatedAt(), ws.conversationId()});
        }
    }

    std::sort(articles.begin(), articles.end(), [](const ArticleEntry &a, const ArticleEntry &b) {
        return a.updatedAt > b.updatedAt;
    });

    const QDate today = QDate::currentDate();
    const QDate yesterday = today.addDays(-1);

    QJsonArray result;
    int count = 0;
    for (const ArticleEntry &entry : articles) {
        if (++count > maxCount)
            break;

        const QDate d = entry.updatedAt.date();
        QString updatedAt;
        if (d == today)
            updatedAt = tr("Today ") + entry.updatedAt.toString("HH:mm");
        else if (d == yesterday)
            updatedAt = tr("Yesterday ") + entry.updatedAt.toString("HH:mm");
        else
            updatedAt = entry.updatedAt.toString(tr("MMM d HH:mm"));

        QJsonObject obj;
        obj[STR_KEY_ID]             = entry.id;
        obj[STR_KEY_NAME]           = entry.name;
        obj[STR_KEY_UPDATED_AT]     = updatedAt;
        obj[STR_KEY_CONVERSATION_ID] = entry.conversationId;
        result.append(obj);
    }

    return QJsonDocument(result).toJson(QJsonDocument::Compact);
}

QString AIWriter::getWritingTemplates()
{
    static QString cached;
    if (!cached.isEmpty())
        return cached;

    // 中文系统加载 zh_CN，其他语言均使用 en_US
    const QString locale = QLocale::system().name().simplified();
    const QString langSuffix = locale.startsWith("zh") ? "zh_CN" : "en_US";
    const QString resourcePath = QString(":/assets/assistants/ai-writing/writing-templates/writing-templates-%1.json").arg(langSuffix);
    const QString iconBasePath = "qrc:///assets/assistants/ai-writing/writing-templates/icons/";

    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(logAssistant) << "AIWriter: failed to open writing templates:" << resourcePath;
        return "[]";
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        qCWarning(logAssistant) << "AIWriter: failed to parse writing templates:" << parseError.errorString();
        return "[]";
    }

    // 将 icon 字段由图标名解析为完整的 qrc:// 路径
    QJsonArray templates = doc.array();
    for (int i = 0; i < templates.size(); ++i) {
        QJsonObject tpl = templates[i].toObject();
        const QString iconName = tpl.value(STR_KEY_ICON).toString();
        if (!iconName.isEmpty())
            tpl[STR_KEY_ICON] = iconBasePath + iconName + ".svg";
        templates[i] = tpl;
    }

    cached = QJsonDocument(templates).toJson(QJsonDocument::Compact);
    return cached;
}

void AIWriter::cancel()
{
    emit requestCancel();
}

void AIWriter::ensureWorkspace()
{
    if (m_workspace.isValid())
        return;

    // Try to load existing workspace by conversation ID
    if (m_conversation) {
        QString convId = m_conversation->id();
        WritingWorkspace existing = WorkspaceStore::instance()->findByConversationId(convId);
        if (existing.isValid()) {
            m_workspace = existing;
            return;
        }
        // Create new workspace
        m_workspace.setConversationId(convId);
    }
}

QVariantHash AIWriter::run()
{
    QVariantHash result;

    if (!m_conversation) {
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidAssistant;
        m_error[STR_KEY_MESSAGE] = "No conversation set";
        return result;
    }

    auto modelVendor = ModelVendor::instance();
    ModelAccountPtr account = modelVendor->getModel(m_modelId);

    if (!account.constData()) {
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "No model found for id: " + m_modelId;
        qCWarning(logAssistant) << "AIWriter: no model found for id:" << m_modelId;
        return result;
    }

    auto model = modelVendor->createModel(account).dynamicCast<AbstractChatModel>();
    if (model.isNull()) {
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "Failed to create model";
        qCWarning(logAssistant) << "AIWriter: failed to create model for account:" << account->id;
        return result;
    }

    // Ensure workspace exists
    ensureWorkspace();

    // Create orchestrator in this worker thread
    WritingOrchestrator agent;
    connect(this, &AIWriter::requestCancel, &agent, &WritingOrchestrator::cancel, Qt::DirectConnection); // must be DirectConnection
    agent.initialize();
    agent.setModel(model);
    agent.setWorkspace(&m_workspace);

    QVariantHash modelParams;
    modelParams[STR_KEY_STREAM] = true;
    modelParams[STR_KEY_THINKING] = m_parameters.value(STR_KEY_THINKING, false).toBool();
    agent.setModelParams(modelParams);

    // Build message history from conversation
    QString currentMsgId = m_conversation->currentMessage();
    QList<MessageNodePtr> history = m_conversation->history(currentMsgId);
    Q_ASSERT(!history.isEmpty());

    MessageNodePtr question = history.takeLast();
    Q_ASSERT(question->getId() == currentMsgId);

    QList<ModelMessage> historyMsg;
    ModelMessage currentMessage;

    for (auto node : history)
        historyMsg.append(node->getMessage());

    {
        auto qmsg = question->getMessage();
        currentMessage = qmsg.takeLast();
        historyMsg.append(qmsg);
    }

    QVariantHash agentParams = m_parameters;

    // Forward render messages to session layer
    connect(&agent, &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "AIWriter render:" << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash response = agent.processRequest(currentMessage, historyMsg, agentParams);

    // Persist workspace (stages already updated m_workspace in-place)
    WorkspaceStore::instance()->save(m_workspace);

    // Propagate errors and content
    m_error = agent.lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}
