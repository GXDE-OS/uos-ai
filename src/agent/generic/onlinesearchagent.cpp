#include "onlinesearchagent.h"
#include "research/retrievers/searchenginefactory.h"
#include "conversation/messagenode.h"
#include "global_define.h"
#include "agentstep.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QLoggingCategory>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QApplication>
#include <QThread>
#include <QFile>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

OnlineSearchAgent::OnlineSearchAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "OnlineSearchAgent";
    m_description = "Online search agent that determines whether web search is needed";
    m_systemPrompt = R"(You are an intelligent assistant that determines whether a user's question requires web search.
**Today** is %1.

You have access to two tools:
1. **websearch** – Use this tool when you are NOT absolutely certain you can answer the question accurately from your internal knowledge alone.
2. **finish** – Use this tool ONLY when ALL of the following conditions are met:
   - The question is about stable, timeless knowledge (e.g., basic greetings, simple arithmetic, fundamental physics laws)
   - OR the question asks about a well-established historical event with a definitive answer
   - AND you are highly confident that your knowledge is complete, accurate, and up-to-date on this topic
   - AND the answer would not benefit from any recent updates, changes, or developments

**You MUST use websearch for:**
- Any question about current events, recent news, or ongoing situations
- Anything involving real-time or frequently changing data (prices, exchange rates, weather, rankings, schedules, patch notes, game meta/strategies, sports scores, stock prices)
- Questions about specific products, software, games, or entertainment where details may have changed (new versions, patches, updates, new episodes)
- Topics involving politics, policies, regulations, or international affairs
- Questions about people (celebrities, politicians, public figures) and their current status
- Any topic where being thorough and accurate matters more than speed
- Recommendations, comparisons, or "how-to" questions where recent information could improve the answer
- ANY question where you have even slight uncertainty about the accuracy or completeness of your knowledge
- When the user explicitly asks you to search, gather, or look up information.

**Use finish ONLY for clearly static topics, such as:**
- Simple greetings (e.g., "hello", "how are you")
- Basic math or logic (e.g., "what is 2+2")
- Fundamental science that has not changed (e.g., "what is gravity")
- Well-established historical facts with definitive answers (e.g., "who was the first person to walk on the moon")
- General programming syntax or concepts that are stable (e.g., "how to write a for loop in Python")
- Creative writing, role-playing, or emotional support conversations
- Direct system commands (e.g., "open Bluetooth", "switch wallpaper", "send an email", "launch application X") — these are action requests, not knowledge queries, so no search is needed

**IMPORTANT RULES:**
- When in doubt, ALWAYS choose websearch. It is better to search unnecessarily than to provide outdated or incorrect information.
- You MUST call EXACTLY ONE of these two tools – either websearch OR finish.
- Do NOT provide any text response before or after calling the tool.
- Do NOT call any other tools.
- Your response must be a single tool call, nothing else.

**Examples:**

| User question | Tool call | Reason |
|---------------|-----------|--------|
| "When was company X founded?" | websearch | Factual but verify for accuracy |
| "What is company X's stock price today?" | websearch | Real-time data |
| "How to write a for loop in Python?" | finish | Stable programming knowledge |
| "What major news happened recently?" | websearch | Current events |
| "Hello, how are you?" | finish | Simple greeting |
| "What are the best rune builds for game mode X?" | websearch | Game meta changes frequently |
| "Did politician Y visit country Z recently?" | websearch | Current political event |
| "What is the boiling point of water?" | finish | Stable scientific fact |
| "Recommend some good phones" | websearch | Recommendations need current info |
| "Explain quantum entanglement" | finish | Fundamental physics, stable knowledge |
| "Turn on Bluetooth for me" | finish | Direct system command, not a knowledge query |
| "How do I switch the wallpaper?" | websearch | How-to question, search for better guidance |
| "Search for X" | websearch | User explicitly requests a search |
| "Look up what's new in Python 3.12" | websearch | User asks to look up information |

**Key principle:** Default to websearch. Only skip search for trivially simple, timeless questions where you have zero doubt about the accuracy of your answer.

Your response must be a single tool call, nothing else.)";
}

OnlineSearchAgent::~OnlineSearchAgent()
{

}

bool OnlineSearchAgent::initialize()
{
    createTools();
    return LlmAgent::initialize();
}

QString OnlineSearchAgent::systemPrompt() const
{
    return m_systemPrompt.arg(QDateTime::currentDateTime().toString(tr("yyyy-MM-dd ddd (year-month-day week)")));
}

QVariantHash OnlineSearchAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &agentParams)
{
    if (agentParams.contains(STR_KEY_MAX_TOKENS)) {
        int max = agentParams[STR_KEY_MAX_TOKENS].toInt();
        if (max > 1)
            maxContentSize = max;
    }

    auto ret = LlmAgent::processRequest(question, history, agentParams);
    ret["search_content"] = searchContent;

    // 遇到错误或无内容
    if (searchContent.isEmpty()) {
        auto msg = makeSearchStep("", SsFailed);
        emit messageReceived(msg);
    }

    return ret;
}

void OnlineSearchAgent::cancel()
{
    LlmAgent::cancel();
    emit agentCancled();
}

void OnlineSearchAgent::createTools()
{
    m_tools.clear();

    // websearch 工具
    ModelTool websearchTool;
    websearchTool.name = "websearch";
    websearchTool.description = "Search the web for information. Use when you need current or specific information.";
    ModelToolProperty queryProp;
    queryProp.name = "query";
    queryProp.type = "string";
    queryProp.description = "The search keywords to use for finding information";
    websearchTool.properties.append(queryProp);
    websearchTool.required.append("query");
    m_tools.append(websearchTool);

    // finish 工具
    ModelTool finishTool;
    finishTool.name = "finish";
    finishTool.description = "End the conversation without web search. Use when no web search is needed.";
    m_tools.append(finishTool);
}

QPair<int, QString> OnlineSearchAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    QPair<int, QString> result;

    if (toolName == "websearch") {
        QString query = params.value("query").toString();
        if (query.isEmpty()) {
            result = qMakePair(-1, QString("Error: search query is empty"));
        } else {
            // 正在搜索相关资料
            {
                auto msg = makeSearchStep(tr("Searching for relevant information"), SsSearching);
                emit messageReceived(msg);
            }

            QJsonArray info;
            auto searchResult = performWebSearch(query, &info);

            if (canceled || searchResult.first < 0) {
                // skip
            } else {
                if (!info.isEmpty()) {
                    //参考 n 篇资料
                    auto msg = makeSearchStep(tr("References %0 sources").arg(info.size()), SsCompleted, info);
                    emit messageReceived(msg);
                    searchContent = searchResult.second;
                }
            }

            result = qMakePair(-1, QString("Searched content size: %0").arg(searchResult.second.size()));
        }
    } else if (toolName == "finish") {
        // finish 工具直接返回 -1 结束
        result = qMakePair(-1, QString("finish"));
    } else {
        result = qMakePair(-1, QString("Unknown tool: %1").arg(toolName));
    }

    return result;
}

QPair<int, QString> OnlineSearchAgent::performWebSearch(const QString &query, QJsonArray *page)
{
    qCInfo(logAgent) << "Performing web search for:" << query;

    // 使用 SearchEngineFactory 创建火山搜索引擎
    auto searchEngine = SearchEngineFactory::create(SearchEngineFactory::EngineType::Volcano);
    if (!searchEngine) {
        qCWarning(logAgent) << "Failed to create search engine";
        return {-1, QString("{\"error\": \"Failed to create search engine\"}")};
    }

    connect(this, &OnlineSearchAgent::agentCancled, searchEngine.data(), &SearchEngine::requestAbort);
    if (canceled)
        return {-1, "user canceled"};

    // 执行搜索，获取网页列表
    QJsonArray searchResults = searchEngine->search(query, 10);

    if (searchResults.isEmpty()) {
        qCWarning(logAgent) << "No search results found";
        return {1, QString("{\"error\": \"No search results found\"}")};
    }
    // 已找到 n 个网页
    {
        auto msg = makeSearchStep(tr("%0 pages found").arg(searchResults.size()), SsSearching);
        emit messageReceived(msg);
        QThread::msleep(200);
    }

#if 0
    {
        QFile file(":/assets/agent/Readbody.js");
        file.open(QIODevice::ReadOnly);
        readability = QString::fromUtf8(file.readAll());
    }

    // 并行抓取网页内容，每批最多 maxParallel 个
    int logicalCores = QThread::idealThreadCount();
#if defined (Q_PROCESSOR_X86)|| defined (Q_PROCESSOR_X86_64)
    const int maxParallel = std::min(std::max(logicalCores, 1), 8);
#elif defined (Q_PROCESSOR_ARM_64)
    const int maxParallel = std::min(std::max(logicalCores, 1), 4);
#else
    const int maxParallel = std::min(std::max(logicalCores, 1), 2);
#endif
    QJsonArray enrichedResults;
    int idx = 1;
    int charCount = 0;
    QSet<QString> readed;
    int resultIdx = 0;

    while (resultIdx < searchResults.size() && !canceled && charCount <= maxContentSize && enrichedResults.size() <= 10) {
        struct FetchTask {
            QString url;
            QString title;
            WebSearchContext *context = nullptr;
        };

        QVector<FetchTask> batch;
        while (resultIdx < searchResults.size() && batch.size() < maxParallel) {
            QJsonObject resultObj = searchResults[resultIdx].toObject();
            QString url = resultObj.value("href").toString();
            resultIdx++;

            if (readed.contains(url) || url.isEmpty())
                continue;

            readed.insert(url);

            FetchTask task;
            task.url = url;
            task.title = resultObj.value("title").toString();
            task.context = new WebSearchContext;
            task.context->moveToThread(qApp->thread());
            task.context->setJs(readability);
            batch.append(task);
        }

        if (batch.isEmpty())
            break;

        // 显示浏览状态
        {
            auto msg = makeSearchStep(tr("Browsing %0").arg(
                batch[0].title.isEmpty() ? tr("page %0").arg(0) : batch[0].title), SsReading);
            emit messageReceived(msg);
        }

        qCInfo(logAgent) << "Begin parallel fetch batch of" << batch.size() << "pages";

        // 启动并行抓取，使用单个事件循环等待所有结果
        QEventLoop loop;
        int remaining = batch.size();
        connect(this, &OnlineSearchAgent::agentCancled, &loop, &QEventLoop::quit);

        for (int i = 0; i < batch.size(); i++) {
            connect(batch[i].context, &WebSearchContext::finished, &loop, [this, &loop, &remaining, i, &batch]() {
                if (i != 0) {
                    auto msg = makeSearchStep(tr("Browsing %0").arg(
                        batch[i].title.isEmpty() ? tr("page %0").arg(0) : batch[i].title), SsReading);
                    emit messageReceived(msg);
                }

                if (--remaining <= 0)
                    loop.quit();
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(batch[i].context, "fetchContent", Qt::QueuedConnection,
                                      Q_ARG(QString, batch[i].url));
        }

        QTimer timer;
        timer.setInterval(5000);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();

        if (!canceled) {
            loop.exec();
        } else {
            qCInfo(logAgent) << "User canceled the search before fetch all pages";
        }
        timer.stop();

        // 收集抓取结果
        for (int i = 0; i < batch.size(); i++) {
            QString content = batch[i].context->getContent();
            batch[i].context->deleteLater();
            batch[i].context = nullptr;

            if (canceled || charCount > maxContentSize)
                continue;

            content = QJsonDocument::fromJson(content.toUtf8()).object().value("content").toString();
            if (content.trimmed().isEmpty())
                continue;

            if (content.length() > 6000)
                content = content.left(6000) + "...";

            qCInfo(logAgent) << "get content from" << batch[i].url << "size:" << content.size()
#ifdef QT_DEBUG
                            << content
#endif
                                ;


            QJsonObject enrichedResult;
            if (page) {
                enrichedResult["url"] = batch[i].url;
                enrichedResult["title"] = batch[i].title;
                page->append(enrichedResult);
                enrichedResult = QJsonObject();
            }

            enrichedResult["index"] = idx++;
            enrichedResult["content"] = content;
            enrichedResults.append(enrichedResult);
            charCount += content.size();
        }
    }
#else
    QJsonArray enrichedResults;
    int charCount = 0;
    QSet<QString> readed;
    int resultIdx = 0;

    for (int i = 0; i < searchResults.size() && !canceled && charCount <= maxContentSize; ++i) {
        auto resultObj = searchResults[i].toObject();
        QString url = resultObj.value("href").toString();
        QString title = resultObj.value("title").toString();;

        if (readed.contains(url) || url.isEmpty())
            continue;

        readed.insert(url);

        // 显示浏览状态
        {
            auto msg = makeSearchStep(tr("Browsing %0").arg(title.isEmpty() ? tr("page %0").arg(i + 1) : title), SsReading);
            emit messageReceived(msg);
        }

        QString content = resultObj.value("summary").toString().trimmed();
        if (content.isEmpty())
            content = resultObj.value("content").toString().trimmed();

        if (content.isEmpty())
            continue;

        if (content.length() > 6000)
            content = content.left(6000) + "...";

        QJsonObject enrichedResult;
        if (page) {
            enrichedResult["url"] = url;
            enrichedResult["title"] = title;
            page->append(enrichedResult);
            enrichedResult = QJsonObject();
        }

        enrichedResult["index"] = resultIdx++;;
        enrichedResult["content"] = content;
        enrichedResults.append(enrichedResult);

        charCount += content.size();
        QThread::msleep(50);
    }
#endif
    QJsonDocument doc(enrichedResults);
    return {0, QString::fromUtf8(doc.toJson(QJsonDocument::Compact))};
}

WebSearchContext::WebSearchContext(QObject *parent) : QObject (parent)
{

}

WebSearchContext::~WebSearchContext()
{

}

QString WebSearchContext::getContent()
{
    QMutexLocker lk(&mtx);
    return content;
}

void WebSearchContext::setJs(const QString &text)
{
    js = text;
}

void WebSearchContext::fetchContent(const QString &url)
{
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(logAgent) << "fetchContent must run in main thread.";
        return;
    }

    auto page = createWebPage();
    page->load(QUrl(url));
    connect(page, &QWebEnginePage::loadFinished, this, [page, this](bool success) {
        if (success) {
            if (js.isEmpty()) {
                // 提取网页文本内容 - toPlainText 是异步的
                page->toPlainText([this](const QString &text) {
                    {
                        QMutexLocker lk(&mtx);
                        content = text;
                    }
                    emit this->finished();
                });
            } else {
                page->runJavaScript(js, [this](const QVariant &v) {
                        {
                            QMutexLocker lk(&mtx);
                            content = v.toString();
                        }
                            emit this->finished();
                        });
            }
        } else {
            emit this->finished();
        }
    });
}

QWebEnginePage *WebSearchContext::createWebPage()
{
    auto webPage = new HeadlessWebEnginePage(this);
    webPage->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    webPage->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, false);
    webPage->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
    webPage->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    webPage->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    webPage->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, false);
    webPage->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
    webPage->settings()->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain, false);
    webPage->settings()->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, false);

    return webPage;
}

HeadlessWebEnginePage::~HeadlessWebEnginePage()
{

}

QStringList HeadlessWebEnginePage::chooseFiles(QWebEnginePage::FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes)
{
    qCWarning(logAgent) << "reject websearch choose files, mode:" << mode << "oldFiles:" << oldFiles << "acceptedMimeTypes:" << acceptedMimeTypes;
    return {};
}

QWebEnginePage *HeadlessWebEnginePage::createWindow(QWebEnginePage::WebWindowType type)
{
    qCWarning(logAgent) << "reject websearch create window, type:" << type;
    return nullptr;
}

void HeadlessWebEnginePage::javaScriptAlert(const QUrl &securityOrigin, const QString &msg)
{
    qCWarning(logAgent) << "reject websearch alert, securityOrigin:" << securityOrigin << msg;
}

bool HeadlessWebEnginePage::javaScriptConfirm(const QUrl &securityOrigin, const QString &msg)
{
    qCWarning(logAgent) << "reject websearch confirm, securityOrigin:" << securityOrigin << msg;
    return false;
}

bool HeadlessWebEnginePage::javaScriptPrompt(const QUrl &securityOrigin, const QString &msg, const QString &defaultValue, QString *result)
{
    qCWarning(logAgent) << "reject websearch prompt, securityOrigin:" << securityOrigin << msg << defaultValue; 
    return false;
}
