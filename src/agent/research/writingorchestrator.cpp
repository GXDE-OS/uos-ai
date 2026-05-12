#include "writingorchestrator.h"
#include "writingmode.h"
#include "writingstage.h"
#include "outlinemode.h"
#include "articlemodel.h"
#include "tools/researchtools.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

WritingOrchestrator::WritingOrchestrator(QObject *parent)
    : LlmAgent(parent)
{
    m_name        = "WritingOrchestrator";
    m_description = "Orchestrates the full AI writing workflow via mode-based pipeline.";
}

WritingOrchestrator::~WritingOrchestrator()
{
    delete m_mode;
}

bool WritingOrchestrator::initialize()
{
    m_mode = new OutlineMode();
    return true;
}

void WritingOrchestrator::setWorkspace(WritingWorkspace *workspace)
{
    m_workspace = workspace;
}

void WritingOrchestrator::cancel()
{
    LlmAgent::cancel();
    if (m_activeStage)
        m_activeStage->cancel();
}

// ---------------------------------------------------------------------------
// processRequest: main entry point
// ---------------------------------------------------------------------------

QVariantHash WritingOrchestrator::processRequest(const ModelMessage &question,
                                                  const QList<ModelMessage> &history,
                                                  const QVariantHash &params)
{
    canceled = false;

    if (!m_mode) {
        qCWarning(logResearch) << "WritingOrchestrator: no writing mode set";
        return QVariantHash();
    }

    if (!m_workspace) {
        qCWarning(logResearch) << "WritingOrchestrator: no workspace set";
        return QVariantHash();
    }

    // Parse upload files immediately and store as workspace references,
    // so subsequent turns (ResearchStage) can access file content without
    // needing the original file paths.
    QStringList uploadFiles = params.value(STR_RESEARCH_UPLOAD_FILES).toStringList();
    if (!uploadFiles.isEmpty()) {
        QJsonArray fileRefs = parseFilesToReferences(uploadFiles);
        if (!fileRefs.isEmpty())
            m_workspace->appendReferences(fileRefs);
    }

    // Ensure an active article exists; create one if none
    // Title will be set from the outline's top-level title after OutlineStage runs
    if (!m_workspace->hasArticles()) {
        m_workspace->addArticle(QString());
    }

    // Step 1: Classify intent
    QString userIntent = classifyIntent(question);

    qCInfo(logResearch) << "WritingOrchestrator: intent =" << userIntent
                     << "| hasArticle =" << m_workspace->hasArticleContent()
                     << "| articleCount =" << m_workspace->articleCount();

    // Step 2: Create stage sequence via WritingMode
    QList<WritingStage*> stages = m_mode->createStages(*m_workspace, question, userIntent, this);

    if (stages.isEmpty()) {
        qCWarning(logResearch) << "WritingOrchestrator: no stages created";
        return QVariantHash();
    }

    // Step 3: Execute stages sequentially
    QVariantHash result;
    QList<ModelMessage> mergedContext;

    for (WritingStage *stage : stages) {
        if (canceled) break;

        qCInfo(logResearch) << "WritingOrchestrator: executing stage" << stage->id();

        stage->setModel(m_llm);
        stage->setModelParams(m_modelParams);
        connect(stage, &WritingStage::messageReceived,
                this, &WritingOrchestrator::messageReceived);

        m_activeStage = stage;

        // Build stage params: merge caller params with workspace transients
        QVariantHash stageParams = params;

        result = stage->execute(*m_workspace, question, history, stageParams);

        m_activeStage = nullptr;

        // Accumulate assistant messages from stages that contribute to context
        if (stage->contributeToContext()) {
            QList<ModelMessage> ctx = result.value(STR_KEY_CONTEXT).value<QList<ModelMessage>>();
            for (const auto &msg : ctx) {
                if (msg.role == STR_KEY_ASSISTANT)
                    mergedContext.append(msg);
            }
        }

        // Check for errors
        if (!stage->lastError().isEmpty() && stage->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
            qCWarning(logResearch) << "WritingOrchestrator: stage" << stage->id() << "error:" << stage->lastError();
            break;
        }

        // Interactive stage: stop pipeline, wait for user
        if (stage->isInteractive())
            break;
    }

    // Replace context with merged result from all contributing stages
    if (!mergedContext.isEmpty())
        result[STR_KEY_CONTEXT] = QVariant::fromValue(mergedContext);

    // Cleanup stages
    qDeleteAll(stages);

    return result;
}

// ---------------------------------------------------------------------------
// parseFilesToReferences: register upload files as reference objects
// ---------------------------------------------------------------------------

QJsonArray WritingOrchestrator::parseFilesToReferences(const QStringList &files)
{
    QJsonArray refs;
    for (const QString &filePath : files) {
        QString content = ResearchTools::readDocument(filePath);
        if (content.isEmpty()) {
            qCWarning(logResearch) << "WritingOrchestrator: failed to parse upload file snippet:" << filePath;
            continue;
        }

        QFileInfo fi(filePath);
        QJsonObject ref;
        ref[STR_KEY_URL]     = filePath;
        ref[STR_KEY_TITLE]   = fi.fileName();
        ref[STR_KEY_CONTENT] = content;
        ref[STR_KEY_SNIPPET] = content.left(200);
        ref[STR_KEY_WEBSITE] = QStringLiteral("Local Assets");
        ref[STR_KEY_ICON]    = ResearchTools::getFileIconKey(filePath);
        refs.append(ref);
        qCInfo(logResearch) << "WritingOrchestrator: parsed upload file:" << fi.fileName()
                            << "chars:" << content.size();
    }
    return refs;
}

// ---------------------------------------------------------------------------
// classifyIntent: LLM-based intent classification
// ---------------------------------------------------------------------------

QString WritingOrchestrator::classifyIntent(const ModelMessage &msg)
{
    const Article *art = m_workspace->activeArticle();

    // Fast path: no prior context (no article content and no outline)
    if (!art || (!art->hasContent() && !art->hasOutline())) {
        qCInfo(logResearch) << "WritingOrchestrator: no prior context, skip classification -> outline";
        return QString();
    }

    if (!m_llm) {
        qCWarning(logResearch) << "WritingOrchestrator: m_llm is null, skip classification";
        return QString();
    }

    QString userMessage = msg.content.value(0).data.toString();

    // Build state description for the classifier
    QString stateDesc;
    QString contextHint;
    if (m_workspace->hasArticleContent()) {
        stateDesc = "The article has been written and is available for adjustment.";
        if (art) {
            stateDesc += QString(" Current version: %1 (total versions: %2).")
                             .arg(art->currentVersionNumber())
                             .arg(art->versionCount());
            ArticleModel model = ArticleModel::fromMarkdown(art->currentContent());
            if (!model.isEmpty()) {
                QStringList sectionTitles;
                for (const auto &sec : model.sections())
                    sectionTitles << QString("[ID: %1] %2").arg(sec.id, sec.title);
                contextHint = QString("Article sections:\n%1").arg(sectionTitles.join("\n"));
            }
        }
    } else if (art && art->hasOutline()) {
        stateDesc = "An outline has been confirmed but the article has not been written yet.";
        QString outlineMd;
        ResearchTools::outlineJson2Md(art->outline(), 1, outlineMd);
        if (!outlineMd.isEmpty())
            contextHint = QString("Current outline (summary):\n%1").arg(outlineMd.left(600));
    }

    QString classifyPrompt = QString(
R"(You are a writing workflow router. Analyze the user's message and output a single JSON object.

Current writing state: %1
%2
User message: "%3"

Determine the routing intent:
- "outline"     : User wants to generate, modify, or entirely redo the document outline.
- "research"    : User approves/confirms the outline and wants to START writing the full article.
- "adjust"      : User wants to modify existing article content.
- "rewrite"     : User wants to completely rewrite the article (new outline + research + compose).
- "rollback"    : User wants to revert to a previous version of the article.
- "new_article" : User wants to create a new, separate article within the same workspace.
- "chat"        : User is asking a question, requesting an explanation, or having a general conversation.

Rules:
1. If the user says "start writing" or similar confirmation of the outline -> "research".
2. If the user explicitly requests changes to the outline structure -> "outline".
3. If the user explicitly requests changes to the article body -> "adjust".
4. If the user says "rewrite", "redo from scratch", "start over" -> "rewrite".
5. If the user says "rollback", "undo", "go back to previous version" -> "rollback".
6. If the user says "write another article", "new article" -> "new_article".
7. If the user is asking, explaining, chatting, or giving feedback without requesting a concrete writing action -> "chat".
8. When uncertain, prefer "chat" over switching the writing stage.

Respond with ONLY valid JSON, no extra text. Example: {"intent": "chat"})"
    ).arg(stateDesc, contextHint, userMessage);

    QList<ModelMessage> classifyMessages;
    ModelMessage sysMsg;
    sysMsg.role    = STR_KEY_SYSTEM;
    sysMsg.content = {{ContentType::CntText,
                       "You are a precise JSON routing classifier for a writing assistant workflow. Output only valid JSON."}};
    classifyMessages.append(sysMsg);

    ModelMessage userMsg;
    userMsg.role    = STR_KEY_USER;
    userMsg.content = {{ContentType::CntText, classifyPrompt}};
    classifyMessages.append(userMsg);

    QVariantHash output = m_llm->chatCompletion(classifyMessages, QVariantHash());

    if (!m_llm->lastError().isEmpty() && m_llm->lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
        qCWarning(logResearch) << "WritingOrchestrator: classification LLM error, fallback";
        return QString();
    }

    QString content = output.value(STR_KEY_CONTENT).toString().trimmed();

    // Strip markdown code fence
    static QRegularExpression fenceRe(R"(```(?:json)?\s*([\s\S]*?)\s*```)",
                                      QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch fenceMatch = fenceRe.match(content);
    if (fenceMatch.hasMatch())
        content = fenceMatch.captured(1).trimmed();

    int braceStart = content.indexOf('{');
    int braceEnd   = content.lastIndexOf('}');
    if (braceStart != -1 && braceEnd > braceStart)
        content = content.mid(braceStart, braceEnd - braceStart + 1);

    QJsonParseError parseError;
    QJsonDocument intentDoc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    QJsonObject intentObj = intentDoc.object();
    QString intent = intentObj.value("intent").toString().toLower().trimmed();

    if (intentDoc.isNull() || intent.isEmpty()) {
        qCWarning(logResearch) << "WritingOrchestrator: failed to parse classification JSON:"
                               << parseError.errorString() << "raw:" << content.left(200);
    }

    qCInfo(logResearch) << "WritingOrchestrator: classification result intent =" << intent;

    return intent;
}

} // namespace uos_ai
