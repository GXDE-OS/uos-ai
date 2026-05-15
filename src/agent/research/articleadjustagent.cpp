#include "articleadjustagent.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QUuid>
#include <QDateTime>
#include <QRegularExpression>
#include <QLocale>

namespace uos_ai {

ArticleAdjustAgent::ArticleAdjustAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "ArticleAdjustAgent";
    m_description = "Agent for modifying articles via tool calls.";

    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";
    m_systemPrompt = QString(R"(You are a writing assistant. Apply the user's change using exactly one tool call. Before the tool call output one short sentence; after it output one short sentence.
- Use `rename_article` to change only the article's main title — never touches body content.
- Use `rename_section` to change only a section heading — never touches its body content.
- Use `edit_section` to rewrite a section's body text only — never changes the heading.
- Use `rewrite_article` for broad changes that span multiple sections or restructure the whole article.
Never include [ID:…] markers in any field. Respond in %1.
)").arg(language);

    m_tools.append({
        STR_RESEARCH_TOOL_RENAME_ARTICLE,
        "Change only the article's main title (H1 heading). Does not modify any body content.",
        {
            {STR_KEY_TITLE, "string", "The new article title (plain text, no # prefix)."},
        },
        {STR_KEY_TITLE}
    });

    m_tools.append({
        STR_RESEARCH_TOOL_RENAME_SECTION,
        "Change only a section's heading text. Does not modify the section body.",
        {
            {STR_RESEARCH_SECTION_ID, "string", "The [ID: xxx] of the section to rename."},
            {STR_KEY_TITLE,           "string", "The new heading text (plain text, no # prefix)."},
        },
        {STR_RESEARCH_SECTION_ID, STR_KEY_TITLE}
    });

    m_tools.append({
        STR_RESEARCH_TOOL_EDIT_SECTION,
        "Rewrite the body of a single section. Does not change the section heading.",
        {
            {STR_RESEARCH_SECTION_ID, "string", "The [ID: xxx] of the section whose body to rewrite."},
            {STR_KEY_CONTENT,         "string", "New body text (plain Markdown, no heading line)."},
        },
        {STR_RESEARCH_SECTION_ID, STR_KEY_CONTENT}
    });

    m_tools.append({
        STR_RESEARCH_TOOL_REWRITE_ARTICLE,
        "Replace the entire article. Use only when changes span multiple sections or restructure the article.",
        {
            {STR_KEY_CONTENT, "string", "The complete rewritten article."},
        },
        {STR_KEY_CONTENT}
    });
}

ArticleAdjustAgent::~ArticleAdjustAgent()
{
}

QSharedPointer<LlmAgent> ArticleAdjustAgent::create()
{
    return QSharedPointer<LlmAgent>(new ArticleAdjustAgent());
}

QVariantHash ArticleAdjustAgent::processRequest(const ModelMessage &question,
                                                 const QList<ModelMessage> &history,
                                                 const QVariantHash &params)
{
    m_article      = params.value(STR_RESEARCH_ARTICLE_CONTENT).toString();
    m_articleId    = params.value(STR_KEY_ID).toString();
    m_articleModel = ArticleModel::fromMarkdown(m_article);

    // Inject current article (with section IDs) into the system prompt
    QString articleForPrompt = m_articleModel.isEmpty() ? m_article : m_articleModel.toAnnotatedMarkdown();
    m_systemPrompt += QString("\n\n# Current Article\n\n%1").arg(articleForPrompt);

    m_modelParams[STR_KEY_STREAM] = true;
    QVariantHash response = LlmAgent::processRequest(question, history, params);
    response[STR_RESEARCH_CLEAN_ARTICLE] = m_pendingArticle;

    return response;
}

QPair<int, QString> ArticleAdjustAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName != STR_RESEARCH_TOOL_RENAME_ARTICLE  &&
        toolName != STR_RESEARCH_TOOL_RENAME_SECTION  &&
        toolName != STR_RESEARCH_TOOL_EDIT_SECTION    &&
        toolName != STR_RESEARCH_TOOL_REWRITE_ARTICLE)
        return qMakePair(-1, QString("Unknown tool: %1").arg(toolName));

    // Remove all tools immediately to prevent the LLM from calling a second time
    m_tools.clear();

    // Strip [ID: xxx] markers the LLM may have copied from the annotated prompt
    static QRegularExpression idMarkerRe(R"(\s*\[ID:\s*[^\]]+\])");

    QString finalArticle;

    if (toolName == STR_RESEARCH_TOOL_RENAME_ARTICLE) {
        QString title = params.value(STR_KEY_TITLE).toString().trimmed();
        if (title.isEmpty())
            return qMakePair(-1, QString("Error: title is empty."));

        if (!m_articleModel.isEmpty()) {
            // Find the H1 section and update its title
            const auto &sections = m_articleModel.sections();
            if (!sections.isEmpty() && sections.first().level == 1)
                m_articleModel.updateSectionTitle(sections.first().id, title);
            finalArticle = m_articleModel.toMarkdown();
        } else {
            // Fallback: replace the H1 line in raw markdown
            static QRegularExpression h1Re(R"(^#\s+.*$)", QRegularExpression::MultilineOption);
            finalArticle = m_article;
            finalArticle.replace(h1Re, QString("# %1").arg(title));
        }

    } else if (toolName == STR_RESEARCH_TOOL_RENAME_SECTION) {
        QString sectionId = params.value(STR_RESEARCH_SECTION_ID).toString().trimmed();
        QString title     = params.value(STR_KEY_TITLE).toString().trimmed();
        if (sectionId.isEmpty())
            return qMakePair(-1, QString("Error: section_id is required for rename_section."));
        if (title.isEmpty())
            return qMakePair(-1, QString("Error: title is empty."));

        if (!m_articleModel.isEmpty()) {
            m_articleModel.updateSectionTitle(sectionId, title);
            finalArticle = m_articleModel.toMarkdown();
        } else {
            finalArticle = m_article;
        }

    } else if (toolName == STR_RESEARCH_TOOL_EDIT_SECTION) {
        QString sectionId = params.value(STR_RESEARCH_SECTION_ID).toString().trimmed();
        QString content   = params.value(STR_KEY_CONTENT).toString().trimmed();
        if (sectionId.isEmpty())
            return qMakePair(-1, QString("Error: section_id is required for edit_section."));
        if (content.isEmpty())
            return qMakePair(-1, QString("Error: content is empty."));

        content.replace(idMarkerRe, QString());

        if (!m_articleModel.isEmpty()) {
            if (content == STR_RESEARCH_DELETE_SECTION)
                m_articleModel.deleteSection(sectionId);
            else
                m_articleModel.updateSection(sectionId, content);
            finalArticle = m_articleModel.toMarkdown();
        } else {
            finalArticle = content;
        }

    } else {
        // rewrite_article
        QString content = params.value(STR_KEY_CONTENT).toString().trimmed();
        if (content.isEmpty())
            return qMakePair(-1, QString("Error: content is empty."));
        content.replace(idMarkerRe, QString());
        finalArticle = content;
    }

    QString title;
    static QRegularExpression titleRe(R"(^#\s+(.*)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = titleRe.match(finalArticle);
    if (match.hasMatch()) {
        title = match.captured(1).trimmed();
    } else {
        // Fallback: extract from original article (e.g. edit_section with empty model)
        QRegularExpressionMatch origMatch = titleRe.match(m_article);
        title = origMatch.hasMatch() ? origMatch.captured(1).trimmed() : tr("Untitled");
    }

    m_pendingArticle = finalArticle;

    return qMakePair(0, QString("Article updated."));
}

} // namespace uos_ai
