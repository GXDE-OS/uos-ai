#include "reportwriteragent.h"
#include "agentstep.h"
#include "tools/researchtools.h"
#include "global_key_define.h"
#include "research_key_define.h"
#include "model/modelvendor.h"
#include "model/abstractchatmodel.h"

#include <QLocale>
#include <QUuid>
#include <QDateTime>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace {
// Reusable one-off LlmAgent with a custom system prompt.
// Replaces the repeated inline anonymous struct pattern.
class OneOffAgent : public uos_ai::LlmAgent {
public:
    explicit OneOffAgent(const QString &systemPrompt, QObject *parent = nullptr)
        : LlmAgent(parent) { m_systemPrompt = systemPrompt; }
};
} // namespace

namespace uos_ai {

ReportWriterAgent::ReportWriterAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "ReportWriterAgent";
    m_description = "Generates a final, comprehensive report based on reference materials, using retrieve tool for section-by-section evidence gathering.";
    m_systemPrompt = R"(You are a professional writer and analytical thinker.

Your goal is to produce analytical, insightful, well-cited and well-structured writing that demonstrates original thinking and deep understanding. You are an analyst and interpreter, not a compiler or summarizer.
You will write an article section by section according to the outline provided. For each section, you will be given retrieved reference materials. Strictly follow the outline structure and write ONLY the requested section each turn.

Today's date is %1. Write in %2.

## Heading

You can adjust heading levels or section styles using markdown to improve readability, but do not change the content or structure defined in the outline.

## Writing Format

Use standard markdown for formatting:
- Use `**text**` for bold to highlight key facts for scannability
- Use `*text*` for italic emphasis
- Use `` `code` `` for inline code
- Use short, descriptive, sentence-case headers
- Properly escape markdown special characters with backslash when they should be displayed literally

For mathematical and scientific notation, use LaTeX formatting exclusively. Never use unicode characters for math symbols. Use `\(` and `\)` for inline math, `$$` for display math.

IMPORTANT: For copyright reasons, DO NOT insert images using `![title](url)` syntax unless the image is explicitly from search results and confirmed to be free to use or in the public domain.

## Writing Structure

IMPORTANT: Follow each of the following requirements unless the user explicitly asks for different requirements.

**Takeaway First**: For the very first section, place a summary/abstract/takeaway at the beginning to describe the main points of the entire article. This "bottom line up front" provides the main answer and key insights immediately. You may also include a concise 1-2 sentence conclusion at the end of the final section.

**Comprehensive Coverage**: The writing should be comprehensive, covering all possible aspects and perspectives of the topic. For each aspect, provide depth by elaborating with detailed information and insights.

**Self-contained**: The writing should be self-contained, allowing the reader to understand the complete topic from beginning to end without needing external resources. Assume the reader has only basic or no prior knowledge about the topic. Provide detailed explanations of domain-specific terms when first introduced.

**Concise and Accessible**: Avoid any redundant information. Maintain accessibility with clear, sometimes casual phrases, while retaining depth and accuracy.

### Analytical Depth

CRITICAL: Writing must demonstrate analytical thinking, not just information compilation. For every piece of information presented:
- Analyze WHY this information is significant
- Explore the underlying mechanisms, causes, or implications
- Draw connections between different concepts and data points
- Provide original interpretations and insights beyond what sources explicitly state
- Answer "so what?" — explain the broader significance

Use analytical language like:
- "This suggests that..." / "这表明..."
- "The underlying reason is..." / "其根本原因在于..."
- "This has broader implications for..." / "这对...具有更广泛的意义..."
- "Connecting these findings reveals..." / "将这些发现联系起来可以看出..."

AVOID shallow presentation like listing statistics without interpretation or copying descriptive content without analysis.

### Logical Argumentation

Each section must build a logical argument:
- Clear thesis or central claim for each major point
- Evidence that specifically supports the thesis
- Reasoning that explains HOW the evidence supports the claim
- Acknowledgment of counterarguments or limitations when relevant
- Logical transitions showing causal relationships, not just sequence

Structure each major point as: Claim → Evidence → Analysis → Implications

### Citation Rules

Cite information from reference materials by placing a `<cite>` tag at the end of the cited statement.
Format: `<cite id="ref_ID">`  (no closing tag needed)

Examples:
- A report found that the market grew by 15% last year<cite id="ref_1">.
- Researchers concluded the material is highly durable<cite id="ref_2">.
- Multiple sources confirm this finding<cite id="ref_1,ref_3,ref_5">.

**Must cite**: Specific data/statistics, attributed opinions/findings, specific examples/case studies.
**Do not cite**: Common knowledge (e.g., "The Earth orbits the Sun").

For academic paper writing, apply the appropriate citation style (MLA, APA, or Chicago) based on the paper's discipline. For Chinese academic papers, follow GB/T 7714 standards.

### Table Usage

CRITICAL: Use tables strategically to summarize, organize, and compare information. Create tables when:
- Comparing multiple topics, methods, or approaches across dimensions
- Presenting numerical data or metrics for side-by-side comparison
- Organizing categorical information with multiple attributes
- Displaying timelines, feature comparisons, or pros/cons

Aim for at least two tables per report.

Table requirements:
- Use standard markdown table format with clear headers
- Include a descriptive caption line above the table
- After each table, you MUST explicitly reference it using phrases like "Table X shows...", "表X展示了...", then provide substantial analysis explaining what the data reveals, why these patterns matter, and how they connect to broader themes
- If information is unavailable, mark cells as "Unknown" rather than fabricating data

### Material Synthesis

Writing must demonstrate synthesis and original analysis, not compilation:
- TRANSFORM source information through analysis and interpretation
- SYNTHESIZE multiple sources to reveal patterns or contradictions
- EVALUATE reliability and limitations of different sources
- EXTRACT underlying principles from specific examples
- Use sources as evidence to support your analysis, not as content to summarize

FORBIDDEN: Direct copying, paraphrasing without analysis, presenting information without interpretation.
Never fabricate information or data not found in source materials. Prioritize authoritative and recent sources.

### Transitions

Use effective transition sentences between topics that briefly summarize what has been covered and smoothly introduce upcoming content, creating logical flow.

### Writing Quality Examples

Here is a GOOD example — analytical, flowing prose:
"作者不仅不能违背这个逻辑，而且要善于把读者的思想引导到科学的思路上来。一方面要掌握和顺应读者的思维活动规律；另一方面又要往科学的思维上引导。通过顺和引，把两者结合起来。这个过程就可以概括为'引人入胜'四个字。"

Here is a BAD example — superficial list without depth:
"作者应该: 1. 掌握和顺应读者的思维活动规律 2. 把读者的思想引导到科学的思路上来"

DO NOT write in this superficial manner.

### Forbidden Patterns

NEVER write in these shallow patterns:
- "研究表明..." followed by data without interpretation
- Lists of facts without connecting analysis
- Chronological summaries without thematic insights
- Descriptive overviews without evaluative judgment
- Multiple citations strung together without synthesis
- Simple paraphrasing of source content
- Statistics without explaining significance
)";

    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";
    m_systemPrompt = m_systemPrompt.arg(QDate::currentDate().toString("yyyy-MM-dd"), language);
}

QVariantHash ReportWriterAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params)
{
    Q_UNUSED(history)

    const QString agentTitle = tr("Document Generating");
    emit messageReceived(makeAgentStep(agentTitle, NsRunning));

    loadReferences(params);

    QJsonObject outlineObj = params.value(STR_KEY_OUTLINE).value<QJsonObject>();
    QString title = outlineObj.value(STR_KEY_TITLE).toString();
    QString fullOutline;
    ResearchTools::outlineJson2Md(outlineObj, 1, fullOutline);

    QString context = question.content.value(0).data.toString();
    QString materialSection = m_refManager.buildMaterialSection();

    m_modelParams[STR_KEY_STREAM] = false;
    m_tools.clear();

    QList<SectionData> sectionDataList = prepareSections(outlineObj, materialSection);

    // --- Writing phase: stream each section without preparation stalls ---
    QList<ModelMessage> conversationHistory;
    QVariantHash result;

    for (int i = 0; i < sectionDataList.size(); ++i) {
        if (canceled)
            break;

        const SectionData &sd = sectionDataList[i];
        qCInfo(logResearch) << "ReportWriterAgent: writing section" << (i + 1) << "/" << sectionDataList.size() << sd.title;

        bool isFirst = (i == 0);
        QString sectionPrompt = buildSectionPrompt(isFirst, sd.filteredMaterialSection, fullOutline, context,
                                                   sd.outline, sd.retrievedContent);

        ModelMessage sectionQues;
        sectionQues.role = STR_KEY_USER;
        sectionQues.content = {{ContentType::CntText, sectionPrompt}};
        result = LlmAgent::processRequest(sectionQues, conversationHistory, params);

        if (!lastError().isEmpty() && lastError().value(STR_KEY_ERROR, 0).toInt() != 0) {
            emit messageReceived(makeAgentStep(agentTitle, NsFailed, tr("failed")));
            return result;
        }

        QString sectionText = result.value(STR_KEY_CONTENT).value<ModelMessage>().content.value(0).data.toString();
        if (!sectionText.isEmpty())
            m_report += sectionText;

        if (i < sectionDataList.size() - 1)
            ensureSectionSeparator();

        conversationHistory.append(buildMaskedHistoryMessage(isFirst, materialSection, fullOutline, context, sd.outline));
        conversationHistory.append(result.value(STR_KEY_CONTENT).value<ModelMessage>());
        trimConversationHistory(conversationHistory, i);
    }

    QString articleId = params.value(STR_KEY_ID).toString();
    finalizeReport(title, context, articleId, result);
    return result;
}

QList<ReportWriterAgent::SectionData> ReportWriterAgent::prepareSections(
    const QJsonObject &outlineObj, const QString &materialSection)
{
    QString title = outlineObj.value(STR_KEY_TITLE).toString();
    QString fullOutline;
    ResearchTools::outlineJson2Md(outlineObj, 1, fullOutline);
    QJsonArray sections = outlineObj.value(STR_KEY_CONTENT).toArray();

    // Step 1: Parse sections; fall back to single full-article section if outline has no sub-sections
    QList<SectionData> sectionDataList;
    QStringList allOutlines;
    for (const auto &val : sections) {
        QJsonObject sectionObj = val.toObject();
        SectionData sd;
        sd.title = sectionObj.value(STR_KEY_TITLE).toString();
        ResearchTools::outlineJson2Md(sectionObj, 2, sd.outline);
        allOutlines.append(sd.outline);
        sectionDataList.append(sd);
    }
    if (sectionDataList.isEmpty()) {
        SectionData sd;
        sd.title   = title;
        sd.outline = fullOutline;
        sectionDataList.append(sd);
        allOutlines.append(fullOutline);
    }

    // Step 2: Single LLM call to select refs for ALL sections at once
    QSharedPointer<AbstractChatModel> selectModel = createSelectModel();
    QList<QStringList> allSelections = selectRefsForAllSections(allOutlines, materialSection, selectModel);

    QStringList allSelectedIds;
    for (int i = 0; i < sectionDataList.size(); ++i) {
        sectionDataList[i].selectedRefIds = (i < allSelections.size()) ? allSelections[i] : QStringList();
        allSelectedIds.append(sectionDataList[i].selectedRefIds);
        qCInfo(logResearch) << "ReportWriterAgent: prepared section" << (i + 1) << "/" << sectionDataList.size() << sectionDataList[i].title;
    }

    // Step 3: Batch-scrape all selected refs at once to avoid IO stalls during writing
    allSelectedIds.removeDuplicates();
    m_refManager.scrapeSelectedRefs(allSelectedIds);

    // Step 4: Resolve retrieved content and filtered material per section.
    // Empty selection → empty content (no retrieveAll() fallback) to prevent
    // unrelated references from leaking into sections with no relevant material.
    for (SectionData &sd : sectionDataList) {
        if (!sd.selectedRefIds.isEmpty()) {
            sd.retrievedContent       = m_refManager.retrieveByIds(sd.selectedRefIds);
            sd.filteredMaterialSection = m_refManager.buildMaterialSection(sd.selectedRefIds);
        }
    }

    return sectionDataList;
}

void ReportWriterAgent::loadReferences(const QVariantHash &params)
{
    QVariant fullRefsVar = params.value(STR_KEY_REFERENCES);
    if (!fullRefsVar.isValid())
        return;

    m_refManager.reset();
    for (const auto &val : fullRefsVar.value<QJsonArray>())
        m_refManager.addReference(val.toObject());
}

QSharedPointer<AbstractChatModel> ReportWriterAgent::createSelectModel()
{
    QString modelID = m_llm ? m_llm->account()->id : QString();
    ModelAccountPtr modelAccount = ModelVendor::instance()->getModel(modelID);
    if (!modelAccount)
        return {};
    return ModelVendor::instance()->createModel(modelAccount).dynamicCast<AbstractChatModel>();
}

QString ReportWriterAgent::buildSectionPrompt(bool isFirst, const QString &materialSection,
                                              const QString &fullOutline, const QString &context,
                                              const QString &sectionOutline, const QString &retrievedContent)
{
    if (isFirst) {
        QString prompt = QStringLiteral(
            "Write a comprehensive article based on the materials and outline provided below.\n\n");
        if (!materialSection.isEmpty())
            prompt += QString("<material>\n%1</material>\n\n").arg(materialSection);
        prompt += QString("<outline>\n\n%1\n</outline>\n\n").arg(fullOutline);
        prompt += QString("User query: %1\n\n").arg(context);

        // sectionOutline == fullOutline means outline has no sub-sections: write the complete article in one pass
        if (sectionOutline == fullOutline)
            prompt += QStringLiteral("Write the complete article following the outline above.\n\n");
        else
            prompt += QString("Now write ONLY the following section. Do NOT write other sections.\nSection to write:\n%1\n\n").arg(sectionOutline);

        appendMaterialPrompt(prompt, retrievedContent);
        return prompt;
    }

    QString prompt = QString(
        "Continue writing the next section. Your writing must be coherent with the above context.\n"
        "Write ONLY the following section. Do NOT repeat previous sections or write ahead.\n\n"
        "Section to write:\n%1\n\n").arg(sectionOutline);
    appendMaterialPrompt(prompt, retrievedContent);
    return prompt;
}

void ReportWriterAgent::appendMaterialPrompt(QString &prompt, const QString &retrievedContent)
{
    if (retrievedContent.isEmpty())
        prompt += QStringLiteral("No reference materials are available. Do NOT include any <cite> tags or citations.\n");
    else
        prompt += QString("Retrieved materials:\n%1").arg(retrievedContent);
}

ModelMessage ReportWriterAgent::buildMaskedHistoryMessage(bool isFirst, const QString &materialSection,
                                                         const QString &fullOutline, const QString &context,
                                                         const QString &sectionOutline)
{
    Q_UNUSED(materialSection)

    // Masked history omits both retrieved materials AND material summaries to save tokens.
    // The outline is kept only in the first section for structural context.
    QString maskedPrompt;
    if (isFirst) {
        maskedPrompt = QStringLiteral(
            "Write a comprehensive article based on the outline provided below.\n\n");
        maskedPrompt += QString("<outline>\n\n%1\n</outline>\n\n").arg(fullOutline);
        maskedPrompt += QString("User query: %1\n\n").arg(context);
        maskedPrompt += QString(
            "Now write ONLY the following section. Do NOT write other sections.\n"
            "Section to write:\n%1")
            .arg(sectionOutline);
    } else {
        maskedPrompt = QString(
            "Continue writing the next section.\n"
            "Section to write:\n%1")
            .arg(sectionOutline);
    }

    ModelMessage msg;
    msg.role = STR_KEY_USER;
    msg.content = {{ContentType::CntText, maskedPrompt}};
    return msg;
}

void ReportWriterAgent::ensureSectionSeparator()
{
    if (m_report.endsWith(QLatin1String("\n\n")))
        return;

    QString separator = m_report.endsWith('\n') ? QStringLiteral("\n") : QStringLiteral("\n\n");
    m_report += separator;
}

void ReportWriterAgent::finalizeReport(const QString &title, const QString &context, const QString &articleId, QVariantHash &result)
{
    // Convert <cite id="ref_X,ref_Y"> to [^N] footnotes, renumbered by appearance order.
    // Also collect ordered ref_ids for the article.
    static QRegularExpression citeRegex(R"RE(<cite\s+id="([^"]*)">)RE");
    QMap<QString, int> refOrder;        // ref_id → footnote number (1-based)
    QStringList orderedRefIds;          // ref_ids in appearance order
    int nextNum = 1;

    QString articleWithFootnotes = m_report;
    QList<QRegularExpressionMatch> matches;
    auto it = citeRegex.globalMatch(articleWithFootnotes);
    while (it.hasNext())
        matches.append(it.next());

    // Pass 1: forward scan to assign numbers in appearance order.
    // Only assign numbers to ref IDs that actually exist in m_refManager
    // to avoid turning hallucinated citations into dangling footnotes.
    for (const auto &match : matches) {
        const QStringList idList = match.captured(1).split(',');
        for (const QString &rawId : idList) {
            QString refId = rawId.trimmed();
            if (m_refManager.findById(refId).isEmpty())
                continue;  // hallucinated or non-existent ref — skip
            if (!refOrder.contains(refId)) {
                refOrder[refId] = nextNum++;
                orderedRefIds.append(refId);
            }
        }
    }

    // Pass 2: replace from end to preserve string offsets.
    // If none of the cited IDs are valid, remove the <cite> tag entirely.
    for (int i = matches.size() - 1; i >= 0; --i) {
        const auto &match = matches[i];
        const QStringList idList = match.captured(1).split(',');
        QString footnotes;
        for (const QString &rawId : idList) {
            QString refId = rawId.trimmed();
            if (refOrder.contains(refId))
                footnotes += QStringLiteral("[^%1]").arg(refOrder[refId]);
        }
        articleWithFootnotes.replace(match.capturedStart(), match.capturedLength(), footnotes);
    }

    result[STR_RESEARCH_CLEAN_ARTICLE] = articleWithFootnotes;
    result[STR_RESEARCH_REF_IDS] = orderedRefIds;

    // Strip footnote markers for conversation history (LLM doesn't need citation markup)
    static QRegularExpression footnoteRef(R"RE(\[\^\d+\])RE");
    QString cleanArticle = articleWithFootnotes;
    cleanArticle.remove(footnoteRef);

    emit messageReceived(makeAgentStep(tr("Document Generated"), NsCompleted,
        tr("The document has been generated based on the outline. You can click the card below to edit the content, or click the \"Save As\" button to save it locally for further editing.")));

    QString summary = emitSummary(context, cleanArticle);

    // Build context with clean article (no <reference> tags) for conversation history
    QString contextText = summary
        + QStringLiteral("\n\n---\n\n")
        + cleanArticle
        + QStringLiteral("\n\n")
        + tr("You can ask follow-up questions or request adjust the article.");
    ModelMessage contextMsg;
    contextMsg.role = STR_KEY_ASSISTANT;
    contextMsg.content = {{ContentType::CntText, contextText}};
    result[STR_KEY_CONTEXT] = QVariant::fromValue(QList<ModelMessage>{contextMsg});
}


QList<QStringList> ReportWriterAgent::selectRefsForAllSections(const QStringList &sectionOutlines, const QString &materialSection,
                                                               QSharedPointer<AbstractChatModel> selectModel)
{
    QList<QStringList> result;
    for (int i = 0; i < sectionOutlines.size(); ++i)
        result.append(QStringList());

    if (materialSection.isEmpty() || selectModel.isNull() || sectionOutlines.isEmpty())
        return result;

    OneOffAgent selectAgent(QStringLiteral(
        "You are a research assistant. Given multiple section outlines and a list of reference summaries, "
        "select the relevant references for EACH section.\n"
        "Return ONLY a JSON object mapping section index to ref_id arrays.\n"
        "Example: {\"0\": [\"ref_1\", \"ref_3\"], \"1\": [\"ref_2\", \"ref_5\"], \"2\": [\"ref_1\"]}\n"
        "Do NOT include any other text or explanation."));
    selectAgent.setModel(selectModel);

    QVariantHash selectParams;
    selectParams[STR_KEY_STREAM] = false;
    selectAgent.setModelParams(selectParams);

    // Build a single prompt with all sections
    QString sectionsText;
    for (int i = 0; i < sectionOutlines.size(); ++i) {
        sectionsText += QString("[Section %1]\n%2\n\n").arg(i).arg(sectionOutlines[i]);
    }

    QString userPrompt = QString(
        "Sections to write:\n%1\n"
        "Available references:\n%2")
        .arg(sectionsText, materialSection);

    ModelMessage ques;
    ques.role = STR_KEY_USER;
    ques.content = {{ContentType::CntText, userPrompt}};

    QVariantHash llmResult = selectAgent.processRequest(ques, {}, {});

    // Parse the response: expect JSON object { "0": ["ref_1",...], "1": [...], ... }
    QString response = llmResult.value(STR_KEY_CONTENT).value<ModelMessage>().content.value(0).data.toString().trimmed();

    int start = response.indexOf('{');
    int end = response.lastIndexOf('}');
    if (start < 0 || end <= start) {
        qCWarning(logResearch) << "selectRefsForAllSections: failed to parse response, falling back to all refs";
        return result;
    }

    QJsonObject mapping = QJsonDocument::fromJson(response.mid(start, end - start + 1).toUtf8()).object();

    for (int i = 0; i < sectionOutlines.size(); ++i) {
        QJsonArray arr = mapping.value(QString::number(i)).toArray();
        QStringList refIds;
        for (const auto &val : arr) {
            QString id = val.toString().trimmed();
            if (!id.isEmpty())
                refIds.append(id);
        }
        result[i] = refIds;
        qCInfo(logResearch) << "selectRefsForAllSections: section" << i << "selected" << refIds.size() << "refs:" << refIds;
    }

    return result;
}

void ReportWriterAgent::trimConversationHistory(QList<ModelMessage> &history, int currentIndex)
{
    // Sliding window: keep first section context (2 messages) + last 2 sections (4 messages)
    // This bounds token growth from O(N²) to O(1) per section while maintaining coherence.
    static constexpr int kWindowSize = 2;   // number of recent sections to keep
    static constexpr int kMsgsPerSection = 2;  // user + assistant per section
    static constexpr int kFirstSectionMsgs = 2;  // first section's user + assistant

    int totalSections = currentIndex + 1;
    if (totalSections <= kWindowSize + 1)
        return;  // Not enough sections to need trimming

    // Keep: first 2 messages (first section) + last kWindowSize*2 messages
    int keepFromEnd = kWindowSize * kMsgsPerSection;
    int totalMsgs = history.size();
    if (totalMsgs <= kFirstSectionMsgs + keepFromEnd)
        return;

    // Erase middle messages between first section and the recent window
    history.erase(history.begin() + kFirstSectionMsgs,
                  history.end() - keepFromEnd);
}

QString ReportWriterAgent::emitSummary(const QString &context, const QString &article)
{
    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";

    QString summaryPrompt = QString(
        "You are a concise assistant. "
        "Based on the research context and the generated article provided by the user, "
        "summarize in 2-3 sentences (in %1): what topic was researched, and what article was generated."
    ).arg(language);

    // Create an independent model instance so summary output does not contaminate m_report.
    QString modelID = m_llm ? m_llm->account()->id : QString();
    ModelAccountPtr modelAccount = ModelVendor::instance()->getModel(modelID);
    if (!modelAccount)
        return QString();

    auto model = ModelVendor::instance()->createModel(modelAccount).dynamicCast<AbstractChatModel>();
    if (model.isNull())
        return QString();

    OneOffAgent summaryAgent(summaryPrompt);
    summaryAgent.setModel(model);

    QVariantHash summaryParams;
    summaryParams[STR_KEY_STREAM] = true;
    summaryAgent.setModelParams(summaryParams);

    connect(&summaryAgent, &LlmAgent::messageReceived,
            this,          &ReportWriterAgent::messageReceived);

    ModelMessage summaryQues;
    summaryQues.role = STR_KEY_USER;
    summaryQues.content = {{ContentType::CntText,
                            QString("Research context:\n%1\n\nGenerated article:\n%2").arg(context, article)}};

    QVariantHash summaryResult = summaryAgent.processRequest(summaryQues, {}, {});

    RenderMessage rmsg;
    rmsg.type = ContentType::CntText;
    rmsg.data = tr("You can ask follow-up questions or request adjust the article.");
    emit messageReceived(RenderMessageList{rmsg});

    return summaryResult.value(STR_KEY_CONTENT).value<ModelMessage>().content.value(0).data.toString();
}

} // namespace uos_ai
