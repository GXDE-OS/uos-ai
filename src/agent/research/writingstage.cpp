#include "writingstage.h"
#include "writingworkspace.h"
#include "workspacestore.h"
#include "outlineagent.h"
#include "deepresearchagent.h"
#include "reportwriteragent.h"
#include "predictquestion.h"
#include "articleadjustagent.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

// ---------------------------------------------------------------------------
// WritingStage base
// ---------------------------------------------------------------------------

WritingStage::WritingStage(QObject *parent)
    : QObject(parent)
{
}

void WritingStage::setModel(QSharedPointer<AbstractChatModel> llm)
{
    m_llm = llm;
}

void WritingStage::setModelParams(const QVariantHash &params)
{
    m_modelParams = params;
}

void WritingStage::cancel()
{
    m_canceled = true;
    if (m_activeAgent)
        m_activeAgent->cancel();
}

// ---------------------------------------------------------------------------
// OutlineStage
// ---------------------------------------------------------------------------

QVariantHash OutlineStage::execute(WritingWorkspace &ws,
                                    const ModelMessage &userMsg,
                                    const QList<ModelMessage> &history,
                                    const QVariantHash &params)
{
    auto *agent = new OutlineAgent(this);
    agent->initialize();
    agent->setModel(m_llm);
    agent->setModelParams(m_modelParams);
    connect(agent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = agent;

    // Inject articleId so OutlineAgent can emit reference-style outline
    QVariantHash stageParams = params;
    Article *art = ws.activeArticle();
    if (art)
        stageParams[STR_KEY_ID] = art->id();

    QVariantHash result = agent->processRequest(userMsg, history, stageParams);
    m_activeAgent = nullptr;

    if (agent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
        m_lastError = agent->lastError();
        return result;
    }

    // Store outline on the active article and update its title
    QJsonObject outlineObj = result.value(STR_KEY_OUTLINE).value<QJsonObject>();
    if (!outlineObj.isEmpty() && art) {
        art->setOutline(outlineObj);
        QString outlineTitle = outlineObj.value(STR_KEY_TITLE).toString().trimmed();
        if (!outlineTitle.isEmpty())
            art->setTitle(outlineTitle);
    } else if (!outlineObj.isEmpty() && !art) {
        qCWarning(logResearch) << "OutlineStage: active article is null, outline not saved";
    }

    return result;
}

// ---------------------------------------------------------------------------
// ResearchStage
// ---------------------------------------------------------------------------

QVariantHash ResearchStage::execute(WritingWorkspace &ws,
                                     const ModelMessage &userMsg,
                                     const QList<ModelMessage> &history,
                                     const QVariantHash &params)
{
    auto *agent = new DeepResearchAgent(this);
    agent->initialize();
    agent->setModel(m_llm);
    agent->setModelParams(m_modelParams);
    connect(agent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = agent;

    // Build stage-specific params
    QVariantHash stageParams = params;
    Article *art = ws.activeArticle();
    if (art && art->hasOutline())
        stageParams[STR_KEY_OUTLINE] = QVariant::fromValue(art->outline());

    // Pass pre-parsed file references (stored in workspace on first turn) so
    // DeepResearchAgent can seed its ReferenceManager before starting web research.
    QJsonArray existingRefs = ws.references();
    if (!existingRefs.isEmpty())
        stageParams[STR_KEY_REFERENCES] = QVariant::fromValue(existingRefs);

    // Use the initial user task as the question
    QVariantHash result = agent->processRequest(userMsg, history, stageParams);
    m_activeAgent = nullptr;

    if (agent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
        m_lastError = agent->lastError();
        return result;
    }

    // Store references from research (full content for retrieve tool)
    QJsonArray refs = result.value(STR_KEY_REFERENCES).value<QJsonArray>();
    if (!refs.isEmpty())
        ws.setReferences(refs);

    return result;
}

// ---------------------------------------------------------------------------
// ComposeStage (write) — passes reference summaries + retrieve tool
// ---------------------------------------------------------------------------

QVariantHash ComposeStage::execute(WritingWorkspace &ws,
                                    const ModelMessage &userMsg,
                                    const QList<ModelMessage> &history,
                                    const QVariantHash &params)
{
    Q_UNUSED(history)

    auto *agent = new ReportWriterAgent(this);
    agent->initialize();
    agent->setModel(m_llm);
    agent->setModelParams(m_modelParams);
    connect(agent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = agent;

    // Pass original user task as context (not full research content)
    QString userTask = userMsg.content.value(0).data.toString();
    ModelMessage stageInput;
    stageInput.role = STR_KEY_USER;
    stageInput.content = {{ContentType::CntText, userTask}};

    QVariantHash stageParams = params;
    Article *art = ws.activeArticle();
    if (art && art->hasOutline())
        stageParams[STR_KEY_OUTLINE] = QVariant::fromValue(art->outline());
    if (art)
        stageParams[STR_KEY_ID] = art->id();

    // Pass full references (with content) for retrieve tool
    stageParams[STR_KEY_REFERENCES] = QVariant::fromValue(ws.references());

    QVariantHash result = agent->processRequest(stageInput, {}, stageParams);
    m_activeAgent = nullptr;

    if (agent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
        m_lastError = agent->lastError();
        return result;
    }

    // Extract clean article from agent result
    QString articleContent = result.value(STR_RESEARCH_CLEAN_ARTICLE).toString();
    if (articleContent.isEmpty()) {
        articleContent = result.value(STR_KEY_CONTENT)
                            .value<ModelMessage>().content.value(0).data.toString();
    }

    if (!articleContent.isEmpty() && art) {
        if (!articleContent.startsWith(QLatin1String("# ")))
            articleContent = QStringLiteral("# ") + art->title() + QStringLiteral("\n\n") + articleContent;
        art->commitVersion(articleContent, QStringLiteral("initial draft"));

        QStringList refIds = result.value(STR_RESEARCH_REF_IDS).toStringList();
        if (!refIds.isEmpty())
            art->setRefIds(refIds);

        // Persist before emitting DocCard so the editor can read both the article
        // and the workspace references immediately (save() writes workspace.json
        // which contains m_references required to render the reference section).
        WorkspaceStore::instance()->save(ws);

        QVariantHash cardData;
        cardData[STR_KEY_ID]           = art->id();
        cardData[STR_KEY_TITLE]        = art->title();
        cardData[STR_RESEARCH_VERSION] = -1;
        RenderMessage docCard;
        docCard.type = ContentType::CntDocCard;
        docCard.data = cardData;
        emit messageReceived(RenderMessageList{docCard});
    } else if (!articleContent.isEmpty() && !art) {
        qCWarning(logResearch) << "ComposeStage: active article is null, draft content lost";
    }

    return result;
}

// ---------------------------------------------------------------------------
// PredictStage
// ---------------------------------------------------------------------------

QVariantHash PredictStage::execute(WritingWorkspace &ws,
                                    const ModelMessage &userMsg,
                                    const QList<ModelMessage> &history,
                                    const QVariantHash &params)
{
    Q_UNUSED(history)

    auto *agent = new PredictQuestion(this);
    agent->initialize();
    agent->setModel(m_llm);
    agent->setModelParams(m_modelParams);
    connect(agent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = agent;

    // Pass article content via params
    QVariantHash stageParams = params;
    const Article *art = ws.activeArticle();
    stageParams[STR_RESEARCH_ARTICLE_CONTENT] = art ? art->currentContent() : QString();

    QVariantHash result = agent->processRequest(userMsg, {}, stageParams);
    m_activeAgent = nullptr;

    if (agent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0)
        m_lastError = agent->lastError();

    return result;
}

// ---------------------------------------------------------------------------
// AdjustStage
// ---------------------------------------------------------------------------

QVariantHash AdjustStage::execute(WritingWorkspace &ws,
                                   const ModelMessage &userMsg,
                                   const QList<ModelMessage> &history,
                                   const QVariantHash &params)
{
    auto *agent = new ArticleAdjustAgent(this);
    agent->initialize();
    agent->setModel(m_llm);
    agent->setModelParams(m_modelParams);
    connect(agent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = agent;

    Article *art = ws.activeArticle();
    QVariantHash stageParams = params;
    stageParams[STR_RESEARCH_ARTICLE_CONTENT] = art ? art->currentContent() : QString();
    if (art)
        stageParams[STR_KEY_ID] = art->id();

    QVariantHash result = agent->processRequest(userMsg, history, stageParams);
    m_activeAgent = nullptr;

    if (agent->lastError().value(STR_KEY_ERROR, 0).toInt() != 0)
        m_lastError = agent->lastError();

    // Update article with modified content
    QString newArticle = result.value(STR_RESEARCH_CLEAN_ARTICLE).toString();
    if (!newArticle.isEmpty() && art) {
        if (!newArticle.startsWith(QLatin1String("# ")))
            newArticle = QStringLiteral("# ") + art->title() + QStringLiteral("\n\n") + newArticle;
        art->commitVersion(newArticle, QStringLiteral("content adjustment"));

        // Persist before emitting DocCard so the editor can read the file immediately.
        WorkspaceStore::instance()->save(ws);

        QVariantHash cardData;
        cardData[STR_KEY_ID]           = art->id();
        cardData[STR_KEY_TITLE]        = art->title();
        cardData[STR_RESEARCH_VERSION] = -1;
        RenderMessage docCard;
        docCard.type = ContentType::CntDocCard;
        docCard.data = cardData;
        emit messageReceived(RenderMessageList{docCard});
    } else if (!newArticle.isEmpty() && !art) {
        qCWarning(logResearch) << "AdjustStage: active article is null, adjusted content lost";
    }

    return result;
}

// ---------------------------------------------------------------------------
// ChatStage
// ---------------------------------------------------------------------------

QVariantHash ChatStage::execute(WritingWorkspace &ws,
                                 const ModelMessage &userMsg,
                                 const QList<ModelMessage> &history,
                                 const QVariantHash &params)
{
    Q_UNUSED(ws)

    if (!m_llm)
        return QVariantHash();

    auto *chatAgent = new LlmAgent(this);
    chatAgent->setModel(m_llm);
    chatAgent->setModelParams(m_modelParams);
    connect(chatAgent, &LlmAgent::messageReceived, this, &WritingStage::messageReceived);

    m_activeAgent = chatAgent;
    QVariantHash result = chatAgent->processRequest(userMsg, history, params);
    m_activeAgent = nullptr;

    return result;
}

// ---------------------------------------------------------------------------
// RollbackStage
// ---------------------------------------------------------------------------

QVariantHash RollbackStage::execute(WritingWorkspace &ws,
                                     const ModelMessage &userMsg,
                                     const QList<ModelMessage> &history,
                                     const QVariantHash &params)
{
    Q_UNUSED(history)

    Article *art = ws.activeArticle();
    if (!art || !art->hasContent()) {
        m_lastError[STR_KEY_ERROR] = 1;
        m_lastError[STR_KEY_MESSAGE] = QStringLiteral("No active article to rollback");
        return QVariantHash();
    }

    // Determine target version from params or user message
    int targetVersion = params.value(STR_RESEARCH_VERSION, -1).toInt();

    if (targetVersion < 0) {
        // Try to extract version number from user message
        QString text = userMsg.content.value(0).data.toString();
        static QRegularExpression versionRe(R"(\d+)");
        QRegularExpressionMatch match = versionRe.match(text);
        if (match.hasMatch()) {
            targetVersion = match.captured(0).toInt();
        } else {
            // Default: rollback to previous version
            targetVersion = art->currentVersionNumber() - 1;
        }
    }

    if (!art->rollbackTo(targetVersion)) {
        m_lastError[STR_KEY_ERROR] = 1;
        m_lastError[STR_KEY_MESSAGE] = QString("Version %1 not found").arg(targetVersion);
        return QVariantHash();
    }

    qCInfo(logResearch) << "RollbackStage: rolled back to version" << targetVersion
                     << "-> new version" << art->currentVersionNumber();

    QVariantHash result;
    result[STR_RESEARCH_ARTICLE_CONTENT] = art->currentContent();
    return result;
}

// ---------------------------------------------------------------------------
// NewArticleStage
// ---------------------------------------------------------------------------

QVariantHash NewArticleStage::execute(WritingWorkspace &ws,
                                       const ModelMessage &userMsg,
                                       const QList<ModelMessage> &history,
                                       const QVariantHash &params)
{
    Q_UNUSED(history)
    Q_UNUSED(params)

    // Extract title from user message or use default
    QString text = userMsg.content.value(0).data.toString();
    QString title = text.left(50).trimmed();
    if (title.isEmpty())
        title = QStringLiteral("New Article");

    QString articleId = ws.addArticle(title);

    qCInfo(logResearch) << "NewArticleStage: created article" << articleId << "title:" << title;

    QVariantHash result;
    result[STR_KEY_ID] = articleId;
    return result;
}

} // namespace uos_ai
