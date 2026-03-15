#include "reportwriteragent.h"
#include "llmagent.h"
#include "tools/researchtools.h"
#include "writingstate.h"
#include "agent/oaifunctionparser.h"
#include "llm/common/networkdefs.h"
#include "reasoninguse.h"
#include "util.h"

#include <QTemporaryFile>
#include <QStandardPaths>
#include <QUuid>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QDebug>

namespace uos_ai {

ReportWriterAgent::ReportWriterAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "ReportWriterAgent";
    m_description = "Generates a final, comprehensive report based on the collected research summaries.";
    m_systemPrompt = R"(You are the **Deep Research Report Writing Master**. Your core task is to write a strictly structured, in-depth, and comprehensive report based on the detailed information provided, strictly following the outline.

**Input Context:**
* **Context:** All research findings and information are provided here.
* **Outline:** Outline content.

**Report Core Requirements:**
1.  **Structural Consistency (Mandatory):** The overall structure (major headings `##` and subheadings `###`) **MUST be strictly generated according to the provided Outline**. You are prohibited from altering the outline's hierarchy or titles.
2.  **Goal and Depth:** The report MUST focus on answering the original task. It must be well-structured, informative, and comprehensive, reaching a **minimum length of 1200 words**. Utilize all relevant facts and figures from the context.
3.  **Tone:** **Objective**, impartial, and professional.

**Formatting Norms (Markdown):**
* Use Markdown headers (`#`, `##`, `###`) strictly matching the outline.
* **Flow Constraint**: **Avoid bullet points**. Prioritize natural, continuous prose (paragraphs). Use Markdown tables only for structured data comparisons.
* **No Table of Contents**.

**Citation Handling:**
* Place citation tags where they naturally support the content. The granularity (sentence, paragraph, or section level) is determined by the content itself — no need to force alignment to any specific boundary.
* The same reference ID **may appear at multiple positions** throughout the document wherever it is relevant.
* **Within a single citation group**, each reference ID must be **unique**. Never repeat the same ID in the same group.
 * *Wrong:* `[ref_1][ref_2][ref_1]`
 * *Correct:* `[ref_1][ref_2]`
* Citation tags must be placed **after** the sentence-ending punctuation (e.g., `内容。[ref_1]` or `content.[ref_1]`), never before it (e.g., `内容[ref_1]。` or `content[ref_1].` is wrong).
* Only cite references that are directly relevant to the adjacent content.

**Source Handling Principles:**
* Prioritize **relevance, reliability, and recency**.
* If multiple sources provide the same fact, select the most authoritative one to reduce citation clutter.

**Execution Strategy:**
1.  Read the Outline Node.
2.  Retrieve relevant info from Context.
3.  Synthesize the text into smooth paragraphs.
4.  **Citation Step:** When citing, place `[ref_N]` tags where they naturally fit the content. Within any single citation group, ensure each ID is unique.
)";

    QString language = Util::checkLanguage() ? "Chinese" : "English";
    m_systemPrompt += QString("\n\nIn %1!\n").arg(language);
}

QJsonObject ReportWriterAgent::processRequest(const QJsonObject &question, const QJsonArray &message, const QVariantHash &params)
{
    ReasoningUse::reasoningUseTitle(this, tr("Document Generated"), ReasoningUse::InProgress);
    QString context = question.value("content").toString();
    QJsonArray references = question.value("references").toArray();

    // 从 WritingState 读取大纲（QJsonObject，无需经过 QVariantHash 转换）
    WritingState state = WritingState::fromParams(params);
    QJsonObject outlineObj = state.outline();
    QString title = outlineObj.value("title").toString();
    QString outline;
    outlineJson2Md(outlineObj, 1, outline);

    QJsonObject ques;
    ques["role"] = "user";
    ques["content"] = QString("Context: %1\n\nOutline:\n%2").arg(context, outline);

    m_streamBuffer.clear();

    auto messages = initChatMessages(ques, message);
    m_llm->switchStream(true);
    connect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, [this, references](const QString &content) {
        if (canceled) {
            qDebug() << "received delta content after abort." << content;
            return;
        }

        QString contentStr = QJsonDocument::fromJson(content.toUtf8()).object().value("message").toObject().value("content").toString();

        m_streamBuffer.append(contentStr);

        // Find complete tags and send reference
        static QRegularExpression refRegex(R"(\[ref_(\d+)\])");
        auto i = refRegex.globalMatch(m_streamBuffer);
        while (i.hasNext()) {
            auto match = i.next();
            int idx = match.captured(1).toInt() - 1;
            if (idx >= 0 && idx < references.size()) {
                sendReference(references.at(idx).toObject());
            }
        }
        m_streamBuffer.replace(refRegex, "");

        QString toEmit;
        int lastBracket = m_streamBuffer.lastIndexOf('[');
        if (lastBracket != -1) {
            QString suffix = m_streamBuffer.mid(lastBracket);
            // Check if it matches start of [ref_...
            static QRegularExpression partialRegex(R"(^\[(r(e(f(_\d*)?)?)?)?$)" );
            if (partialRegex.match(suffix).hasMatch()) {
                // It is a potential partial tag, keep it in buffer
                toEmit = m_streamBuffer.left(lastBracket);
                m_streamBuffer = suffix;
            } else {
                // Not a partial tag we care about
                toEmit = m_streamBuffer;
                m_streamBuffer.clear();
            }
        } else {
            toEmit = m_streamBuffer;
            m_streamBuffer.clear();
        }

        if (toEmit.isEmpty()) {
            return;
        }

        QJsonObject message;
        message.insert("content", toEmit);
        message.insert("chatType", ChatAction::ChatTextPlain);
        QJsonObject wrapper;
        wrapper.insert("message", message);
        wrapper.insert("stream", true);

        OutputCtx ctx;
        ctx.deltaContent = QJsonDocument(wrapper).toJson();
        bool ret = handleStreamOutput(&ctx);
        if (!ret) {
            canceled = true;
            // cancle
            m_llm->aborted();
        }
    });

    QJsonObject output = m_llm->predict(QJsonDocument(messages).toJson(), m_tools);
    disconnect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, nullptr);

    if (m_llm->lastError() != AIServer::NoError) {
        return output;
    }

    QString report = output.value("content").toString();
    // Also remove tags from the final report
    static QRegularExpression refRegex(R"(\[ref_\d+\])");
    report.replace(refRegex, "");
    
    docCardContent(title, report);

    ReasoningUse::reasoningUseTitle(this, tr("Document Generated"), ReasoningUse::Completed);
    ReasoningUse::reasoningUseContent(this, tr("The document has been generated based on the outline. You can click the card below to edit the content, or click the \"Save As\" button to save it locally for further editing."));
    return output;
}

void ReportWriterAgent::outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown)
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

void ReportWriterAgent::docCardContent(const QString &title, const QString &report)
{
    QJsonObject message;

    QJsonObject contentObj;
    contentObj.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    contentObj.insert("title", title);
    contentObj.insert("content", report);

    message.insert("content", contentObj);
    message.insert("chatType", ChatAction::ChatDocCard); // 任务进度状态

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

void ReportWriterAgent::sendReference(const QJsonObject &reference)
{
    QJsonObject message;
    message.insert("content", "<reference>"+QString::fromUtf8(QJsonDocument(reference).toJson())+"</reference>");
    message.insert("chatType", ChatAction::ChatTextPlain); // 任务进度状态

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

} // namespace uos_ai

