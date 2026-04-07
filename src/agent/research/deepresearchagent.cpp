#include "deepresearchagent.h"
#include "deepanalyzer.h"
#include "tools/webscraper.h"
#include "tools/researchtools.h"
#include "writingstate.h"
#include "retrievers/searchenginefactory.h"
#include "tooluse.h"
#include "reasoninguse.h"
#include "embeddingserver.h"
#include "networkdefs.h"
#include "util.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QDate>
#include <QUuid>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

namespace uos_ai {

inline constexpr char kSearchTitle[] = "title";
inline constexpr char kSearchSource[] = "source";
inline constexpr char kSearchContent[] = "content";

DeepResearchAgent::DeepResearchAgent(QObject *parent) : LlmAgent(parent)
{
    m_refOffset = 1;

    m_name = "deep_research_agent";
    m_description = "Deep Research Orchestrator & Executor";
    m_systemPrompt = R"(# Role: Advanced Deep Research Orchestrator

**Objective**: You are a high-autonomy research engine. Your mission is to transform a vague query or a structured outline into a comprehensive, high-density knowledge base. You operate with a "depth-first" mindset, ensuring no stone is left unturned before concluding the operation.

---

### OPERATIONAL PROTOCOLS

#### 1. Requirement-Driven Execution (Precision Targeting)
* **Targeted Mapping**: Your primary task is to deconstruct the user’s initial query or outline into specific information nodes (e.g., technical parameters, market share, historical context).
* **Direct Retrieval**: Launch high-relevance searches targeting each missing link in the outline. Avoid generic background filler; aim directly for core facts and high-density data.
* **Contextual Branching**: Pivot to branched searches only if you discover peripheral information that is critical to understanding the core subject matter.

#### 2. Dynamic Sufficiency Assessment (The "Stop" Rule)
* **Sufficiency Check**: After each round of results, immediately assess if the gathered information is sufficient to cover the specific section of the outline with high-density, professional content.
* **Efficiency Finish**: Call `research_finish` **immediately** once the collected data (metrics, dates, core mechanisms, key entities, etc.) can support a comprehensive report. Do not perform redundant or low-value crawls simply to increase search volume.
* **Gap-Only Iteration**: If an evaluation reveals a weak sub-point, subsequent searches must target **only that specific gap** with surgical precision, rather than restarting the general research process.

#### 3. Search Query Engineering (High-Density Retrieval)
* **Specificity**: Never use generic queries. You must use "long-tail keywords" and technical jargon (e.g., use "NVIDIA H100 GPU architecture memory bandwidth analysis" instead of "NVIDIA GPU details").
* **Diversity & Pivot**: If a search path returns redundant information, pivot your strategy immediately. Use opposing viewpoints, technical whitepapers (`filetype:pdf`), or industry-specific terminology to ensure both breadth and depth.
* **Language Constraint**: All generated search queries and keywords must be written in %1. Do not deviate from this language unless the technical term has no equivalent in that language.

---

**CONSTRAINTS**
1. **Tool-Only Output**: Your response must consist ONLY of the `thought` and the JSON Tool Call. No conversational filler.
2. **Sequential Logic**: Each tool call must be informed by the results of the previous one.
3. **No Assumptions**: If data is missing, search for it. Do not hallucinate facts to fill the outline.

**If you have gathered sufficient, high-density data for every aspect of the request, call `research_finish`. Otherwise, continue the hunt.**)";

    QString language = Util::checkLanguage() ? "Chinese" : "English";
    m_systemPrompt = m_systemPrompt.arg(language);

    QString thoughtDescription = QString(R"(Explain the specific research intent: Identify the exact data gap being targeted, justify why this high-density query was chosen, and describe how this information contributes to reaching the 'Sufficiency' threshold for concluding the research. In %1!)").arg(language);
    QString webToolJson = R"({
        "name": "web_search",
        "description": "Primary engine for external intelligence. Use this to find high-density data, technical specs, and verified metrics. CRITICAL: Do not conclude the research if only generic info is found. If results are vague, you MUST refine queries or switch languages (e.g., English for tech/global, local for regional). Aim for exhaustive coverage of the research mandate.",
        "parameters": {
            "type": "object",
            "required": ["query", "thought"],
            "properties": {
                "query": {
                    "type": "string",
                    "description": "A specific, long-tail query. Use technical jargon or specific years. Avoid broad terms."
                },
                "thought": {
                    "type": "string",
                    "description": ")" + thoughtDescription + R"("
                }
            }
        }
    })";

    QString localToolJson = R"({
        "name": "local_search",
        "description": "Accesses internal 'ground truth' data including emails, internal wikis, and private documents. Use this to find organizational context or proprietary data that supplements public web information. Essential for connecting general research to specific internal projects.",
        "parameters": {
            "type": "object",
            "required": ["query", "thought"],
            "properties": {
                "query": {
                    "type": "string",
                    "description": "Keywords tailored for internal document structures or project names."
                },
                "thought": {
                    "type": "string",
                    "description": ")" + thoughtDescription + R"("
                }
            }
        }
    })";

    QString parserToolJson = R"({
        "name": "parse_upload_file",
        "description": "Deep-dives into specific uploaded artifacts (PDFs, logs, codebases). Use this when you need to extract precise data points, tables, or logic from provided files rather than general summaries.",
        "parameters": {
            "type": "object",
            "required": ["files", "thought"],
            "properties": {
                "files": {
                    "type": "array",
                    "items": { "type": "string" },
                    "description": "List of absolute paths to specific files requiring deep extraction."
                },
                "thought": {
                    "type": "string",
                    "description": ")" + thoughtDescription + R"("
                }
            }
        }
    })";

    QString finishSummaryDescription = QString(R"(A SINGLE, concise sentence in %1. Summarize the research completion and confirm readiness for the writing task. Do not list details.)").arg(language);
    QString finishToolJson = R"({
        "name": "research_finish",
        "description": "The FINAL step. Call this ONLY when every section of the research mandate has been supported by high-quality, verified data. If any section remains vague or lacks specific evidence, you MUST continue using search tools instead of calling this function.",
        "parameters": {
            "type": "object",
            "required": ["summary"],
            "properties": {
                "summary": {
                    "type": "string",
                    "description": ")" + finishSummaryDescription + R"("
                }
            }
        }
    })";

    QJsonDocument webDoc = QJsonDocument::fromJson(webToolJson.toUtf8());
    QJsonDocument localDoc = QJsonDocument::fromJson(localToolJson.toUtf8());
    QJsonDocument parserDoc = QJsonDocument::fromJson(parserToolJson.toUtf8());
    QJsonDocument finishDoc = QJsonDocument::fromJson(finishToolJson.toUtf8());
    m_tools.append(webDoc.object());
    m_tools.append(localDoc.object());
    m_tools.append(parserDoc.object());
    m_tools.append(finishDoc.object());
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

QJsonObject DeepResearchAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    QJsonObject response;
    QString ResearchAgentTitle = tr("Collecting and analyzing data");
    QString ResearchgAgentFinishedTitle = tr("Data collection and analysis completed");
    ReasoningUse::reasoningUseTitle(this, ResearchAgentTitle);

    m_userTask = question.value("content").toString();
    QStringList uploadFiles = params.value("upload_files").toStringList();
    // 从 WritingState 读取大纲（QJsonObject，无 QVariantHash 转换损耗）
    WritingState state = WritingState::fromParams(params);
    QJsonObject outlineObj = state.outline();
    QString outline;
    outlineJson2Md(outlineObj, 1, outline);

    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString userPrompt = QString("Current date: %1\nUser task: %2\n\nOutline:\n%3").arg(currentDate, m_userTask, outline);
    if (!uploadFiles.isEmpty()) {
        userPrompt = userPrompt + QString("\n\nUpload files: %1").arg(uploadFiles.join("\n"));
    } else {
        for (int i = 0; i < m_tools.size(); ++i) {
            if (m_tools.at(i)["name"].toString() == "parse_upload_file") {
                m_tools.removeAt(i);
                break;
            }
        }
    }

    QJsonObject ques;
    ques["role"] = "user";
    ques["content"] = userPrompt;

    QJsonArray originalTools = m_tools;
    if (params.contains("web_search") && !params.value("web_search").toBool()) {
        for (int i = 0; i < m_tools.size(); ++i) {
            if (m_tools.at(i)["name"].toString() == "web_search") {
                m_tools.removeAt(i);
                break;
            }
        }
    }

    // 修复3：若个人知识库为空，移除 local_search 工具，避免对无关内容的无意义调用
    if (EmbeddingServer::getInstance().getDocFiles().isEmpty()) {
        for (int i = 0; i < m_tools.size(); ++i) {
            if (m_tools.at(i)["name"].toString() == "local_search") {
                m_tools.removeAt(i);
                break;
            }
        }
    }

    if (m_tools.size() == 1 && m_tools.first()["name"] == "research_finish") {
        m_tools = QJsonArray();
    }

    response = LlmAgent::processRequest(ques, history, params);
    m_tools = originalTools;

    QString content;
    QJsonArray contextArray = response.value("context").toArray();
    for (auto context : contextArray) {
        QJsonObject contextObj = context.toObject();
        if (contextObj.value("role") == "tool" && !contextObj.value("content").toString().isEmpty()) {
            content = content + contextObj.value("content").toString() + "\n\n";
        }
    }

    if (content.isEmpty()) {
        ReasoningUse::reasoningUseTitle(this, ResearchAgentTitle, ReasoningUse::Failed);
        ReasoningUse::reasoningUseContent(this, tr("No information found."));

        return response;
    } else {
        ReasoningUse::reasoningUseTitle(this, ResearchgAgentFinishedTitle, ReasoningUse::Completed);
    }

    response["content"] = content;
    response["references"] = m_accumulatedReferences;
    return response;
}

QJsonArray DeepResearchAgent::getReferences() const
{
    return m_accumulatedReferences;
}

QPair<int, QString> DeepResearchAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName == "web_search") {
        QString thought = params.value("thought").toString();
        ReasoningUse::reasoningUseContent(this, thought + "\n");

        QJsonArray toolResults = handleWebSearchTool(params);

        if (!toolResults.isEmpty()) {
             QString processContent = handleDeepAnalyzer(toolResults, m_userTask);
            return qMakePair(0, processContent);
        } else {
            return qMakePair(0, QString(""));
        }
    } else if (toolName == "local_search") {
        QString thought = params.value("thought").toString();
        ReasoningUse::reasoningUseContent(this, thought);

        QJsonArray toolResults = handleLocalSearchTool(params);
        if (!toolResults.isEmpty()) {
             QString processContent = addReferencesAndFormat(toolResults);
            return qMakePair(0, processContent);
        } else {
            return qMakePair(0, QString(""));
        }
    } else if (toolName == "parse_upload_file") {
        QString thought = params.value("thought").toString();
        ReasoningUse::reasoningUseContent(this, thought);
        for (int i = 0; i < m_tools.size(); ++i) {
            // 解析上传文件工具只需执行一次
            if (m_tools.at(i)["name"].toString() == "parse_upload_file") {
                m_tools.removeAt(i);
                break;
            }
        }

        QJsonArray toolResults = handleFileParseTool(params);
        if (!toolResults.isEmpty()) {
             QString processContent = addReferencesAndFormat(toolResults);
            return qMakePair(0, processContent);
        } else {
            return qMakePair(0, QString(""));
        }
    } else if (toolName == "research_finish") {
        QString summary = params.value("summary").toString();
        ReasoningUse::reasoningUseContent(this, summary);
        return qMakePair(-1, QString(""));
    }

    return qMakePair(-1, QString("Unknown tool: %1").arg(toolName));
}

QJsonArray DeepResearchAgent::handleWebSearchTool(const QJsonObject &arg)
{
    QString query = arg.value("query").toString();

    ToolUse tool;
    tool.name = "web_search";
    tool.params = query;
    tool.index = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ToolUse::actionUseContent(this, tool);

    QJsonArray searchContent = webSearch(query);
    if (!searchContent.isEmpty()) {
        tool.status = ToolUse::Completed;
        tool.result = tr("Search successful");
        ToolUse::actionUseContent(this, tool);
    } else {
        tool.status = ToolUse::Failed;
        tool.result = tr("No information found!");
        ToolUse::actionUseContent(this, tool);
    }

    return searchContent;
}

QJsonArray DeepResearchAgent::webSearch(const QString &query)
{
    auto searchEngine = SearchEngineFactory::create(SearchEngineFactory::EngineType::Baidu);
    if (!searchEngine) {
        return QJsonArray();
    }

    WebScraper scraper;
    QJsonArray result;

    QJsonArray searchResults = searchEngine->search(query); // serper api search

    for (const QJsonValue &val : searchResults) {
        if (canceled)
            break;

        QJsonObject obj = val.toObject();
        QString urlStr = obj.value("href").toString();
        QString snippet = obj.value("snippet").toString();
        QString scrapedData = scraper.scrape(urlStr);
        if (scrapedData.isEmpty())
            scrapedData = snippet;

        QString website = obj.value("website").toString();
        QString icon = obj.value("icon").toString();
        if (website.isEmpty() && !icon.isEmpty())
            website = urlStr;

        QJsonObject item;
        item.insert("source", urlStr);
        item.insert("title", obj.value("title").toString());
        item.insert("content", scrapedData);
        item.insert("website", website);
        item.insert("icon", icon);
        item.insert("snippet", snippet);
        result.append(item);
    }

    return result;
}

QJsonArray DeepResearchAgent::handleLocalSearchTool(const QJsonObject &arg)
{
    QString query = arg.value("query").toString();

    ToolUse tool;
    tool.name = "local_search";
    tool.params = query;
    tool.index = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ToolUse::actionUseContent(this, tool);

    QJsonArray searchContent = localSearch(query);

    tool.status = ToolUse::Completed;
    tool.result = tr("Search successful");
    ToolUse::actionUseContent(this, tool);

    return searchContent;
}

QJsonArray DeepResearchAgent::localSearch(const QString &query)
{
    // 修复1：distance 为向量距离，值越小表示越相关。
    // 超过此阈值的结果视为与当前任务无关，不纳入引用。
    static constexpr double kMaxLocalSearchDistance = 0.6;

    QJsonArray result;

    QString resultData = EmbeddingServer::getInstance().embeddingSearch(query, 5, AssistantType::AI_WRITING);
    QJsonObject resultObj = QJsonDocument::fromJson(resultData.toUtf8()).object();
    for (auto res : resultObj["result"].toArray()) {
        QJsonObject resObj = res.toObject();

        double distance = resObj.value("distance").toDouble(kMaxLocalSearchDistance + 1.0);
        if (distance > kMaxLocalSearchDistance)
            continue;

        QString filePath = resObj.value("source").toString();
        QString content = resObj.value("content").toString();
        if (content.isEmpty())
            continue;

        QString fileName;
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists())
            fileName = fileInfo.fileName();

        QJsonObject item;
        item.insert("source", filePath);
        item.insert("title", fileName);
        item.insert("content", content);
        item.insert("snippet", content.left(200));
        item.insert("website", tr("Local Knowledge Base"));
        item.insert("icon", ResearchTools::getFileIconData(filePath));
        result.append(item);
    }

    return result;
}

QJsonArray DeepResearchAgent::handleFileParseTool(const QJsonObject &arg)
{
    QJsonArray result;

    QStringList files;
    QJsonArray filesArray = arg.value("files").toArray();
    for (auto file : filesArray) {
        files << file.toString();
    }

    ToolUse tool;
    tool.name = "parse_upload_file";
    tool.params = QJsonDocument(filesArray).toJson();
    tool.index = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ToolUse::actionUseContent(this, tool);

    result = fileParse(files);
    if (!result.isEmpty()) {
        tool.status = ToolUse::Completed;
        tool.result = tr("Material parsing successful");
        ToolUse::actionUseContent(this, tool);
    } else {
        tool.status = ToolUse::Failed;
        tool.result = tr("Material parsing failed!");
        ToolUse::actionUseContent(this, tool);
    }

    return result;
}

QJsonArray DeepResearchAgent::fileParse(const QStringList &files)
{
    QJsonArray result;

    for (const QString &file : files) {
        QString content = ResearchTools::readDocument(file);
        if (content.isEmpty())
            continue;

        QFileInfo fileinfo(file);
        QJsonObject item;
        item.insert("source", file);
        item.insert("title", fileinfo.fileName());
        item.insert("content", content);
        item.insert("snippet", content.left(200));
        item.insert("website", tr("Local Assets"));  // 本地素材
        item.insert("icon", ResearchTools::getFileIconData(file));
        result.append(item);
    }

    return result;
}

QString DeepResearchAgent::handleDeepAnalyzer(const QJsonArray &results, const QString &initTask)
{
    ToolUse analyzerTool;
    analyzerTool.name = "analyzer";
    analyzerTool.params = QString();
    analyzerTool.index = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ToolUse::actionUseContent(this, analyzerTool);

    QString anylyzeContent = deepAnalyze(results, initTask);

    if (!anylyzeContent.isEmpty()) {
        analyzerTool.status = ToolUse::Completed;
        analyzerTool.result = tr("Information analyze successful");
        ToolUse::actionUseContent(this, analyzerTool);
    } else {
        anylyzeContent = tr("No information found!");

        analyzerTool.status = ToolUse::Failed;
        analyzerTool.result = anylyzeContent;
        ToolUse::actionUseContent(this, analyzerTool);
    }

    return anylyzeContent;
}

QString DeepResearchAgent::deepAnalyze(const QJsonArray &searchContent, const QString &task)
{
    // Use QtConcurrent to process each search result in parallel
    QList<QJsonObject> inputList;
    for (const auto &val : searchContent) {
        inputList.append(val.toObject());
    }

    // Capture m_llm and task for the worker
    // Run parallel map using QtConcurrent::run
    QList<QFuture<QJsonObject>> futures;
    for (const auto &input : inputList) {
        futures.append(QtConcurrent::run([this, task, input]() -> QJsonObject {
            QString rawContent = input.value("content").toString();
            
            DeepAnalyzer analyzer;
            analyzer.setModel(this->m_llm); 

            QString userPrompt = QString("User Task:\n%1\n\nRaw Content:\n%2").arg(task, rawContent);
            QJsonObject question;
            question["role"] = "user";
            question["content"] = userPrompt;

            QJsonObject res = analyzer.processRequest(question, QJsonArray(), QVariantHash());
            QString cleanedContent = res.value("content").toString();
            
            if (cleanedContent.isEmpty()) {
                cleanedContent = rawContent; 
            }

            QJsonObject resultObj = input;
            resultObj["cleaned_content"] = cleanedContent;
            return resultObj;
        }));
    }

    QList<QJsonObject> processedResults;
    for (auto &f : futures) {
        processedResults.append(f.result());
    }

    QString finalContent;
    
    // Serial assembly to assign IDs deterministically
    for (const auto &item : processedResults) {
        QString title = item.value("title").toString();
        QString source = item.value("source").toString();
        QString content = item.value("cleaned_content").toString();

        // 去重：同一来源复用已有 refId，不重复写入 m_accumulatedReferences
        QString refId = findRefIdBySource(source);
        if (refId.isEmpty()) {
            refId = QString("ref_%1").arg(m_refOffset++);
            QJsonObject refObj;
            refObj["id"] = refId;
            refObj["title"] = title;
            refObj["url"] = source;
            refObj["website"] = item.value("website").toString();
            refObj["icon"] = item.value("icon").toString();
            refObj["snippet"] = item.value("snippet").toString();
            m_accumulatedReferences.append(refObj);
        }

        // Append to final text
        finalContent += QString("[%1]\nContent: %2\n")
                            .arg(refId, content);
    }

    return finalContent;
}

QString DeepResearchAgent::addReferencesAndFormat(const QJsonArray &results)
{
    QString finalContent;
    
    // Serial assembly to assign IDs deterministically
    for (const auto &val : results) {
        QJsonObject item = val.toObject();
        QString title = item.value("title").toString();
        QString source = item.value("source").toString();
        QString content = item.value("content").toString();

        // 去重：同一来源复用已有 refId，不重复写入 m_accumulatedReferences
        QString refId = findRefIdBySource(source);
        if (refId.isEmpty()) {
            refId = QString("ref_%1").arg(m_refOffset++);
            QJsonObject refObj;
            refObj["id"] = refId;
            refObj["title"] = title;
            refObj["url"] = source;
            refObj["website"] = item.value("website").toString();
            refObj["icon"] = item.value("icon").toString();
            refObj["snippet"] = item.value("snippet").toString();
            m_accumulatedReferences.append(refObj);
        }

        // Append to final text
        finalContent += QString("[%1]\nContent: %2\n")
                            .arg(refId, content);
    }

    return finalContent;
}

QString DeepResearchAgent::findRefIdBySource(const QString &source) const
{
    for (const auto &val : m_accumulatedReferences) {
        QJsonObject ref = val.toObject();
        if (ref.value("url").toString() == source)
            return ref.value("id").toString();
    }
    return QString();
}

void DeepResearchAgent::outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown)
{
    QString title = outlineObj["title"].toString().trimmed();

    if (!title.isEmpty()) {
        QString headerPrefix;
        for (int i = 0; i < level; ++i)
            headerPrefix += "#";
        markdown += QString("%1 %2\n\n").arg(headerPrefix, title);
    }

    if (outlineObj.contains("content") && outlineObj["content"].isArray()) {
        QJsonArray contentArray = outlineObj["content"].toArray();
        for (const QJsonValue &childValue : contentArray)
        {
            outlineJson2Md(childValue.toObject(), level + 1, markdown);
        }
    }
}

} // namespace uos_ai
