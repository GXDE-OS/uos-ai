#include "outlineagent.h"
#include "agentstep.h"
#include "tools/researchtools.h"
#include "retrievers/searchenginefactory.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QLocale>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

OutlineAgent::OutlineAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "OutLineAgent";
    m_description = "";
    m_systemPrompt = R"(Role: OutlineAgent (Content Architect)

Profile
You are a senior Content Architect and Logic Structuring Expert. Your core capability is creating rigorous, hierarchical outlines from vague inputs.
**Crucial:** Your responsibility is to **Plan**, not **Write**. Even if the user says "Write a novel", you must interpret it as "Generate a plot outline".

Workflow & Constraints
1. **Intent Locking:** Forcefully convert all "writing" requests into "outline generation" requests.
2. **Mode Selection:**
- **Generative Mode:** (User gives topic/keywords) -> Build a logical 3-level skeleton from scratch.
- **Strict Adherence Mode:** (User gives chapters/draft) -> Keep the user's Level 1 structure/order exactly as is, and only expand sub-nodes.
3. **Output Boundary:** STRICTLY JSON format. NO conversational text outside the code block.

Language & Numbering Protocol (CRITICAL)
1. In %1.
2. Apply the specific numbering rules below.

**CASE 1: Default / Chinese Content (Apply this by default)**
If the user input is Chinese, or the target output language is Chinese:
- **Level 1 Headers:** MUST start with Chinese numerals + Dunhao e.g., "一、", "二、", "三、"
- **Level 2 Headers:** MUST start with Arabic numerals + Dunhao e.g., "1、", "2、", "3、"

**CASE 2: English Content (Only if explicitly English)**
If and ONLY if the content is entirely in English:
- **Level 1 Headers:** Roman Numerals (e.g., "I.", "II.").
- **Level 2 Headers:** Capital Letters (e.g., "A.", "B.").

JSON Output Structure
Return only a valid JSON object.
- Fields: Use only `title` (for the header text including number) and `content` (array of children).
- Empty children: Use `[]`.

**Example Template (Follow the Numbering Format Strictly):**
```json
{
 "title": "Main Title of the Article",
 "content": [
     {
         "title": "一、First Section Title",
         "content": []
     },
     {
         "title": "二、Second Section Title",
         "content": [
             {
                 "title": "1、Sub-point A"
             },
             {
                 "title": "2、Sub-point B",
             }
         ]
     },
     {
         "title": "三、Third Section Title",
         "content": [
             {
                 "title": "1、Sub-point C"
             },
             {
                 "title": "2、Sub-point D"
             }
         ]
     }
 ]
})";
    m_systemPrompt = m_systemPrompt.arg(QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English");
}

OutlineAgent::~OutlineAgent()
{

}

bool OutlineAgent::initialize()
{
    return true;
}

QSharedPointer<LlmAgent> OutlineAgent::create()
{
    return QSharedPointer<LlmAgent>(new OutlineAgent());
}

QVariantHash OutlineAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &messages, const QVariantHash &params)
{
    m_modelParams[STR_KEY_STREAM] = false;

    QString outlinePath = params.value(STR_RESEARCH_OUTLINE_PATH).toString();

    QString userText = question.content.value(0).data.toString();
    QString userPrompt;
    QString outlineFinishContent;
    const QString agentTitle = tr("Generating outline");

    if (outlinePath.isEmpty()) {
        emit messageReceived(makeAgentStep(agentTitle, NsRunning, tr("Generating outline content for you.")));
        userPrompt = QString("Task:\n%1").arg(userText);

        // Preliminary search: gather topic context before generating outline
        if (params.value(STR_KEY_ONLINE).toBool() && !userText.isEmpty()) {
            auto searchEngine = SearchEngineFactory::create(SearchEngineFactory::EngineType::Baidu);
            if (!searchEngine) {
                qCWarning(logResearch) << "OutlineAgent: failed to create search engine, skipping preliminary search";
            } else {
                QJsonArray searchResults = searchEngine->search(userText, 8);
                if (!searchResults.isEmpty()) {
                    QString searchContext;
                    for (const auto &val : searchResults) {
                        QJsonObject obj = val.toObject();
                        searchContext += QString("- %1\n  %2\n")
                                             .arg(obj.value(STR_KEY_TITLE).toString(),
                                                  obj.value(STR_KEY_SNIPPET).toString().left(200));
                    }
                    userPrompt += QString("\n\nReference Materials (from preliminary search — use these to inform your outline structure, do NOT copy them):\n%1").arg(searchContext);
                    qCInfo(logResearch) << "OutlineAgent: preliminary search found" << searchResults.size() << "results";
                } else {
                    qCInfo(logResearch) << "OutlineAgent: preliminary search returned no results";
                }
            }
        }

        outlineFinishContent = tr("An editable outline has been generated. After confirming, click the blue button below to proceed to document generation.");
    } else {
        emit messageReceived(makeAgentStep(agentTitle, NsRunning, tr("Detected that you have uploaded a local outline,  Currently parsing the outline content for you.")));
        QString outlineContent = ResearchTools::readDocument(outlinePath);
        if (!outlineContent.isEmpty()) {
            outlineFinishContent = tr("Detected uploaded local outline, please confirm.");
        } else {
            emit messageReceived(makeAgentStep(agentTitle, NsFailed, tr("Failed to parse the uploaded outline file, please re-upload")));
            return QVariantHash();
        }
        userPrompt = QString("Task:\n%1\n\n解析上传的大纲内容：\n%2").arg(userText, outlineContent);
    }

    ModelMessage ques;
    ques.role = STR_KEY_USER;
    ques.content = {{ContentType::CntText, userPrompt}};

    QVariantHash response = LlmAgent::processRequest(ques, messages, params);

    if (!lastError().isEmpty() && lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
        emit messageReceived(makeAgentStep(agentTitle, NsFailed, tr("Generating outline content failed")));
        return response;
    }

    QString content = response.value(STR_KEY_CONTENT).value<ModelMessage>().content.value(0).data.toString();

    QJsonObject outlineObj;
    QRegularExpression regex("```json\\s*([\\s\\S]*?)\\s*```", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = regex.match(content);
    if (match.hasMatch()) {
        outlineObj = QJsonDocument::fromJson(match.captured(1).trimmed().toUtf8()).object();
    } else {
        outlineObj = QJsonDocument::fromJson(content.trimmed().toUtf8()).object();
    }

    emitOutline(outlineObj, params.value(STR_KEY_ID).toString());
    emit messageReceived(makeAgentStep(agentTitle, NsCompleted, outlineFinishContent));

    // Store outline object in result for upstream to update workspace
    if (!outlineObj.isEmpty())
        response[STR_KEY_OUTLINE] = QVariant::fromValue(outlineObj);

    // Replace the raw JSON in both content and context with a human-readable summary,
    // so subsequent LLM calls see coherent conversation history instead of bare JSON.
    if (!outlineObj.isEmpty()) {
        QString title = outlineObj["title"].toString();
        QJsonArray chapters = outlineObj["content"].toArray();

        QString summary;
        if (outlinePath.isEmpty()) {
            summary = tr("Based on your writing task, I have generated the following outline for **%1**:\n\n").arg(title);
        } else {
            summary = tr("I have parsed your uploaded outline and structured it as **%1**:\n\n").arg(title);
        }

        for (int i = 0; i < chapters.size(); ++i) {
            QString chapterTitle = chapters[i].toObject()["title"].toString();
            summary += QString("- %1\n").arg(chapterTitle);

            QJsonArray subs = chapters[i].toObject()["content"].toArray();
            for (int j = 0; j < subs.size(); ++j) {
                summary += QString("  - %1\n").arg(subs[j].toObject()["title"].toString());
            }
        }

        summary += tr("\nThe outline contains %1 sections in total. Please confirm to proceed with research and writing.")
                       .arg(chapters.size());

        // Replace the last assistant message in context so the summary enters
        // conversation history (used by history() in subsequent turns).
        ModelMessage summaryMsg;
        summaryMsg.role = STR_KEY_ASSISTANT;
        summaryMsg.content = {{ContentType::CntText, summary}};

        QList<ModelMessage> context = response.value(STR_KEY_CONTEXT).value<QList<ModelMessage>>();
        if (!context.isEmpty())
            context.last() = summaryMsg;
        response[STR_KEY_CONTEXT] = QVariant::fromValue(context);
    }

    return response;
}

void OutlineAgent::emitOutline(const QJsonObject &outline, const QString &articleId)
{
    QVariantHash data;
    data[STR_KEY_ID]    = articleId;
    data[STR_KEY_TITLE] = outline["title"].toString();

    RenderMessage rmsg;
    rmsg.type = ContentType::CntOutline;
    rmsg.data = data;
    emit messageReceived(RenderMessageList{rmsg});
}

} // namespace uos_ai
