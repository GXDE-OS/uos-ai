#include "outlineagent.h"
#include "tools/researchtools.h"
#include "tooluse.h"
#include "reasoninguse.h"
#include "networkdefs.h"
#include "util.h"

#include <QUuid>
#include <QRegularExpression>

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
}
```)";
    m_systemPrompt = m_systemPrompt.arg(Util::checkLanguage() ? "Chinese" : "English");
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

QJsonObject OutlineAgent::processRequest(const QJsonObject &question, const QJsonArray &messages, const QVariantHash &params)
{
    m_llm->switchStream(false);
    QString outlinePath = params.value("outline_path").toString();

    QString userPrompt;
    QString outlineFinishTitle;
    if (outlinePath.isEmpty()) {
        // 未上传大纲
        ReasoningUse::reasoningUseTitle(this, tr("Generating outline content"));
        userPrompt = QString("Task:\n%1").arg(question.value("content").toString());
        outlineFinishTitle = tr("Outline generated, please confirm");
    } else {
        ReasoningUse::reasoningUseTitle(this, tr("Detected that you have uploaded a local outline,  Currently parsing the outline content for you."));
        QString outlineContent = ResearchTools::readDocument(outlinePath);
        if (!outlineContent.isEmpty()) {
            outlineFinishTitle = tr("Detected uploaded local outline, please confirm.");
        } else {
            ReasoningUse::reasoningUseTitle(this, tr("Failed to parse the uploaded outline file, please re-upload"), ReasoningUse::Failed);
            return QJsonObject();
        }

        userPrompt = QString("Task:\n%1\n\n解析上传的大纲内容：\n%2").arg(question.value("content").toString(), outlineContent);
    }

    QJsonObject ques;
    ques["role"] = "user";
    ques["content"] = userPrompt;

    QJsonObject response = LlmAgent::processRequest(ques, messages, params);

    if (lastError() != AIServer::NoError) {
        ReasoningUse::reasoningUseTitle(this, "", ReasoningUse::Failed);
        ReasoningUse::reasoningUseContent(this, lastErrorString());
        return response;
    }

    QJsonObject outlineObj;
    QRegularExpression regex("```json\\s*([\\s\\S]*?)\\s*```", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = regex.match(response.value("content").toString());
    if (match.hasMatch()) {
        outlineObj = QJsonDocument::fromJson(match.captured(1).trimmed().toUtf8()).object();
    } else {
        outlineObj = QJsonDocument::fromJson(response.value("content").toString().trimmed().toUtf8()).object();
    }

    OutlineContent(outlineObj);

    ReasoningUse::reasoningUseTitle(this, outlineFinishTitle, ReasoningUse::Completed);
    return response;
}

void OutlineAgent::OutlineContent(const QJsonObject &outline)
{
    QJsonObject message;
    message.insert("chatType", ChatAction::ChatOutline);
    message.insert("content", QString::fromUtf8(QJsonDocument(outline).toJson()));

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

} // namespace uos_ai
