#include "deepresearchagent.h"
#include "agentstep.h"
#include "tools/researchtools.h"
#include "retrievers/searchenginefactory.h"
#include "global_key_define.h"
#include "research_key_define.h"
#include "dbus/embeddingserver.h"

#include <QLocale>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDate>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logResearch, "uos-ai.research")

using namespace uos_ai;

DeepResearchAgent::DeepResearchAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "deep_research_agent";
    m_description = "Deep Research Orchestrator & Executor";
    m_systemPrompt = R"(# Role: Advanced Deep Research Orchestrator

**Objective**: You are a high-autonomy research engine. Your mission is to transform a vague query or a structured outline into a comprehensive, high-density knowledge base. You operate with a "depth-first" mindset, ensuring no stone is left unturned before concluding the operation.

---

### OPERATIONAL PROTOCOLS

#### 1. Requirement-Driven Execution (Precision Targeting)
* **Targeted Mapping**: Your primary task is to deconstruct the user's initial query or outline into specific information nodes (e.g., technical parameters, market share, historical context).
* **Direct Retrieval**: Launch high-relevance searches targeting each missing link in the outline. Avoid generic background filler; aim directly for core facts and high-density data.
* **Contextual Branching**: Pivot to branched searches only if you discover peripheral information that is critical to understanding the core subject matter.

#### 2. Dynamic Sufficiency Assessment (The "Stop" Rule)
* **Sufficiency Check**: After each round of results, immediately assess if the gathered information is sufficient to cover the specific section of the outline with high-density, professional content.
* **Efficiency Finish**: Stop calling tools **immediately** once the collected data (metrics, dates, core mechanisms, key entities, etc.) can support a comprehensive report. Do not perform redundant or low-value crawls simply to increase search volume.
* **Gap-Only Iteration**: If an evaluation reveals a weak sub-point, subsequent searches must target **only that specific gap** with surgical precision, rather than restarting the general research process.
* **Depth Commitment**: Do not stop prematurely. Ensure every major section of the outline has substantive, specific data — not just surface-level descriptions. Only conclude when each section has enough concrete facts to support professional-grade writing.

#### 3. Cross-Verification (Data Integrity)
* **Multi-Source Validation**: For critical data points (statistics, dates, technical specifications, market figures), search from at least two different angles or sources to confirm accuracy.
* **Conflict Resolution**: If sources disagree on a key fact, note the discrepancy and search for a more authoritative source (official documentation, government data, peer-reviewed publications) to resolve it.

#### 4. Search Query Engineering (High-Density Retrieval)
* **Specificity**: Never use generic queries. You must use "long-tail keywords" and technical jargon (e.g., use "NVIDIA H100 GPU architecture memory bandwidth analysis" instead of "NVIDIA GPU details").
* **Diversity & Pivot**: If a search path returns redundant information, pivot your strategy immediately. Use opposing viewpoints, technical whitepapers (`filetype:pdf`), or industry-specific terminology to ensure both breadth and depth.
* **Language Constraint**: All generated search queries and keywords must be written in %1. Do not deviate from this language unless the technical term has no equivalent in that language.

---

**CONSTRAINTS**
1. **Concise Narration**: Between tool calls, output 1–3 sentences: briefly note what the last result revealed and what specific gap you are targeting next. No lengthy analysis or conclusions.
2. **Sequential Logic**: Each tool call must be informed by the results of the previous one.
3. **No Assumptions**: If data is missing, search for it. Do not hallucinate facts to fill the outline.
4. **Snippet Awareness**: Search results contain only brief snippets, not full page content. If a snippet mentions relevant information but lacks detail, do NOT conclude the data is unavailable — the full page content will be retrieved later during the writing phase. Treat a relevant snippet as confirmation that the source contains useful information and move on to the next gap.
5. **No Tools Available**: If no tools are provided, analyze the outline based solely on your existing knowledge. Output nothing and stop immediately — do not fabricate research results.
6. **Completion**: When every major section of the outline has been covered with sufficient references, stop calling tools.)";

    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";
    m_systemPrompt = m_systemPrompt.arg(language);

    m_webSearchTool = {
        STR_RESEARCH_TOOL_WEB_SEARCH,
        "Primary engine for external intelligence. Use this to find high-density data, technical specs, and verified metrics. CRITICAL: Do not conclude the research if only generic info is found. If results are vague, you MUST refine queries or switch languages (e.g., English for tech/global, local for regional). Aim for exhaustive coverage of the research mandate.",
        {
            {STR_KEY_QUERY, "string", "A specific, long-tail query. Use technical jargon or specific years. Avoid broad terms."},
        },
        {STR_KEY_QUERY}
    };

    m_localSearchTool = {
        STR_RESEARCH_TOOL_LOCAL_SEARCH,
        "Accesses internal 'ground truth' data including emails, internal wikis, and private documents. Use this to find organizational context or proprietary data that supplements public web information. Essential for connecting general research to specific internal projects.",
        {
            {STR_KEY_QUERY, "string", "Keywords tailored for internal document structures or project names."},
        },
        {STR_KEY_QUERY}
    };

}

DeepResearchAgent::~DeepResearchAgent()
{

}

bool DeepResearchAgent::initialize()
{
    return true;
}

QSharedPointer<LlmAgent> DeepResearchAgent::create()
{
    return QSharedPointer<LlmAgent>(new DeepResearchAgent());
}

QVariantHash DeepResearchAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params)
{
    Q_UNUSED(question)
    Q_UNUSED(history)

    m_modelParams[STR_KEY_STREAM] = true;

    const QString agentTitle = tr("Collecting and analyzing data");
    emit messageReceived(makeAgentStep(agentTitle, NsRunning));

    QVariantHash response;
    m_searchFailCount = 0;
    m_refManager.reset();

    // Seed ReferenceManager with pre-parsed file references (injected by ResearchStage
    // from workspace, which were parsed on the first turn in WritingOrchestrator).
    QJsonArray preRefs = params.value(STR_KEY_REFERENCES).value<QJsonArray>();
    for (const auto &val : preRefs)
        m_refManager.addReference(val.toObject());

    // Build user prompt with original task, outline and date
    QString userPrompt;

    QJsonObject outlineObj = params.value(STR_KEY_OUTLINE).value<QJsonObject>();
    if (!outlineObj.isEmpty()) {
        QString outlineMd;
        ResearchTools::outlineJson2Md(outlineObj, 1, outlineMd);
        userPrompt += QString("\n\nOutline:\n%1").arg(outlineMd);
    }

    userPrompt += QString("\n\nCurrent date: %1").arg(QDate::currentDate().toString("yyyy-MM-dd"));

    ModelMessage ques;
    ques.role = STR_KEY_USER;
    ques.content = {{ContentType::CntText, userPrompt}};

    m_tools.clear();
    if (params.value(STR_KEY_ONLINE).toBool())
        m_tools.append(m_webSearchTool);
//    if (!EmbeddingServer::getInstance().getDocFiles().isEmpty())
//        m_tools.append(m_localSearchTool);

    // If no tools are available, skip the agentic LLM loop entirely.
    // Calling the LLM with an empty tool list risks the model hallucinating
    // tool-call JSON. Use whatever refs were pre-seeded (e.g. uploaded files);
    // if there are none, fail immediately.
    if (m_tools.isEmpty()) {
        QJsonArray refs = m_refManager.references();
        emit messageReceived(makeAgentStep(agentTitle, NsCompleted, tr("Data collection and analysis completed")));
        QString summary = tr("Research completed. %1 references collected:\n").arg(refs.size());
        ModelMessage resultMsg;
        resultMsg.role    = STR_KEY_ASSISTANT;
        resultMsg.source  = name();
        resultMsg.content = {{ContentType::CntText, summary}};
        response[STR_KEY_CONTENT]    = QVariant::fromValue(resultMsg);
        response[STR_KEY_REFERENCES] = QVariant::fromValue(refs);
        return response;
    }

    response = LlmAgent::processRequest(ques, {}, params);

    QJsonArray refs = m_refManager.references();

    emit messageReceived(makeAgentStep(agentTitle, NsCompleted, tr("Data collection and analysis completed")));

    // Build a brief summary as response content for conversation context continuity
    QString summary = tr("Research completed. %1 references collected:\n").arg(refs.size());

    ModelMessage resultMsg;
    resultMsg.role = STR_KEY_ASSISTANT;
    resultMsg.source = name();
    resultMsg.content = {{ContentType::CntText, summary}};
    response[STR_KEY_CONTENT] = QVariant::fromValue(resultMsg);
    response[STR_KEY_REFERENCES] = QVariant::fromValue(refs);
    return response;
}

QPair<int, QString> DeepResearchAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName == STR_RESEARCH_TOOL_WEB_SEARCH) {
        QJsonArray toolResults = handleWebSearchTool(params);

        if (!toolResults.isEmpty()) {
            m_searchFailCount = 0;
            // Return snippets to LLM for research decisions (full content is preserved
            // in ReferenceManager and retrieved during the writing phase)
            QString processContent = m_refManager.formatResults(toolResults, STR_KEY_SNIPPET);
            return qMakePair(0, processContent);
        } else {
            ++m_searchFailCount;
            qCWarning(logResearch) << "Web search failed, query:"
                                   << params.value(STR_KEY_QUERY).toString()
                                   << "consecutive failures:" << m_searchFailCount;
            if (m_searchFailCount >= kMaxSearchFails) {
                qCWarning(logResearch) << "Max consecutive search failures reached, aborting research.";
                return qMakePair(-1, QString("Search unavailable after %1 attempts, stopping.").arg(m_searchFailCount));
            }
            return qMakePair(0, QString("Search returned no results for this query, try a different approach."));
        }
    } else if (toolName == STR_RESEARCH_TOOL_LOCAL_SEARCH) {
        QJsonArray toolResults = handleLocalSearchTool(params);
        if (!toolResults.isEmpty()) {
            QString processContent = m_refManager.formatResults(toolResults, STR_KEY_SNIPPET);
            return qMakePair(0, processContent);
        } else {
            qCDebug(logResearch) << "Local search returned no results, query:"
                                 << params.value(STR_KEY_QUERY).toString();
            return qMakePair(0, QString("No relevant documents found in local knowledge base."));
        }
    }

    return qMakePair(-1, QString("Unknown tool: %1").arg(toolName));
}

QJsonArray DeepResearchAgent::handleWebSearchTool(const QJsonObject &arg)
{
    QString query = arg.value(STR_KEY_QUERY).toString();

    qCDebug(logResearch) << "Web search query:" << query;
    emitAction(STR_RESEARCH_TOOL_WEB_SEARCH, NsRunning, query, QString(), 0);

    QJsonArray searchContent = webSearch(query);
    if (!searchContent.isEmpty()) {
        qCDebug(logResearch) << "Web search succeeded, results:" << searchContent.size() << "query:" << query;
        emitAction(STR_RESEARCH_TOOL_WEB_SEARCH, NsCompleted, query, tr("Search succeeded"), 0);
    } else {
        qCWarning(logResearch) << "Web search returned empty results, query:" << query;
        emitAction(STR_RESEARCH_TOOL_WEB_SEARCH, NsFailed, query, tr("ERROR: No information found!"), 0);
    }

    return searchContent;
}

QJsonArray DeepResearchAgent::webSearch(const QString &query)
{
    auto searchEngine = SearchEngineFactory::create(SearchEngineFactory::EngineType::Baidu);
    if (!searchEngine) {
        qCWarning(logResearch) << "DeepResearchAgent: failed to create search engine";
        return QJsonArray();
    }

    QJsonArray result;

    QJsonArray searchResults = searchEngine->search(query);

    for (const QJsonValue &val : searchResults) {
        if (canceled)
            break;

        QJsonObject obj = val.toObject();
        QString urlStr = obj.value(STR_KEY_HREF).toString();
        QString snippet = obj.value(STR_KEY_SNIPPET).toString();

        // Full content scraping is deferred to the writing phase (ReportWriterAgent)
        // to speed up research and reduce workspace storage size.

        QString website = obj.value(STR_KEY_WEBSITE).toString();
        QString icon = obj.value(STR_KEY_ICON).toString();
        if (website.isEmpty() && !icon.isEmpty())
            website = urlStr;

        QJsonObject item;
        item.insert(STR_KEY_URL, urlStr);
        item.insert(STR_KEY_TITLE, obj.value(STR_KEY_TITLE).toString());
        item.insert(STR_KEY_WEBSITE, website);
        item.insert(STR_KEY_ICON, icon);
        item.insert(STR_KEY_SNIPPET, snippet);
        result.append(item);
    }

    return result;
}

QJsonArray DeepResearchAgent::handleLocalSearchTool(const QJsonObject &arg)
{
    QString query = arg.value(STR_KEY_QUERY).toString();

    emitAction(STR_RESEARCH_TOOL_LOCAL_SEARCH, NsRunning, query, QString(), 0);

    QJsonArray searchContent = localSearch(query);

    if (!searchContent.isEmpty()) {
        emitAction(STR_RESEARCH_TOOL_LOCAL_SEARCH, NsCompleted, query, tr("Search succeeded"), 0);
    } else {
        qCDebug(logResearch) << "Local search returned no results, query:" << query;
        emitAction(STR_RESEARCH_TOOL_LOCAL_SEARCH, NsFailed, query, tr("No relevant documents found"), 0);
    }

    return searchContent;
}

QJsonArray DeepResearchAgent::localSearch(const QString &query)
{
    static constexpr double kMaxLocalSearchDistance = 0.6;

    QJsonArray result;

    QString resultData = EmbeddingServer::getInstance().embeddingSearch(query, 5);
    QJsonParseError parseError;
    QJsonObject resultObj = QJsonDocument::fromJson(resultData.toUtf8(), &parseError).object();
    if (resultObj.isEmpty()) {
        qCWarning(logResearch) << "DeepResearchAgent: localSearch JSON parse failed:"
                               << parseError.errorString() << "raw:" << resultData.left(200);
        return result;
    }
    for (auto res : resultObj[STR_KEY_RESULT].toArray()) {
        QJsonObject resObj = res.toObject();

        double distance = resObj.value(STR_KEY_DISTANCE).toDouble(kMaxLocalSearchDistance + 1.0);
        if (distance > kMaxLocalSearchDistance)
            continue;

        QString filePath = resObj.value(STR_KEY_SOURCE).toString();
        QString content = resObj.value(STR_KEY_CONTENT).toString();
        if (content.isEmpty())
            continue;

        QString fileName;
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists())
            fileName = fileInfo.fileName();

        QJsonObject item;
        item.insert(STR_KEY_URL, filePath);
        item.insert(STR_KEY_TITLE, fileName);
        item.insert(STR_KEY_CONTENT, content);
        item.insert(STR_KEY_SNIPPET, content.left(200));
        item.insert(STR_KEY_WEBSITE, tr("Local Knowledge Base"));
        item.insert(STR_KEY_ICON, ResearchTools::getFileIconKey(filePath));
        result.append(item);
    }

    return result;
}


void DeepResearchAgent::emitAction(const QString &name, NormalStatus status, const QString &params, const QString &result, int index)
{
    QVariantHash data;
    data[STR_KEY_NAME] = name;
    data[STR_KEY_STATUS] = static_cast<int>(status);
    data[STR_KEY_PARAMS] = params;
    data[STR_KEY_RESULT] = result;
    data[STR_KEY_INDEX] = index;

    RenderMessage rmsg;
    rmsg.type = ContentType::CntTool;
    rmsg.data = data;
    emit messageReceived(RenderMessageList{rmsg});
}
