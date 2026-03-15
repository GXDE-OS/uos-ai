#include "articleadjustagent.h"
#include "articlemodificationagent.h"
#include "writingstate.h"
#include "articlemodel.h"

#include <QJsonDocument>
#include <QUuid>
#include <QRegularExpression>
#include <QLocale>

namespace uos_ai {

ArticleAdjustAgent::ArticleAdjustAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "ArticleAdjustAgent";
    m_description = "Agent for modifying articles via tool calls";
    m_systemPrompt = R"(# Role
You are a **Backend Article Adjustment Specialist**.
Your role is to modify articles according to user instructions using the `modify_article` tool.

# System Context (CRITICAL)
When the `modify_article` tool is called, the user's screen **automatically refreshes** to show the new article content.
You **MUST NOT** output the article text or say "Here is the updated version". Doing so causes duplicate content in the UI.

# Response Rules
1. **Action**: Call the `modify_article` tool.
2. **After the tool executes**: Reply with **a single short confirmation sentence** (under 10 words).
3. **Prohibitions**:
 - ❌ NO outputting the article body.
 - ❌ NO saying "Here is the modified version...".
 - ❌ NO summarizing what was changed.

# Examples

**User**: "Remove the third paragraph, it's too long."
**Tool Call**: `modify_article(instruction="delete the third paragraph")`
**AI Output**: Article updated successfully.

**User**: "Change the tone to be more friendly."
**Tool Call**: `modify_article(instruction="rewrite in a friendly tone")`
**AI Output**: Tone adjusted.
)";

    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";
    m_systemPrompt += QString("\nIn %1.\n").arg(language);

    // 工具定义：增加可选 section_id，LLM 通过带 [ID: xxx] 注记的 Markdown 感知章节 ID，
    // 精确指定要修改的章节，避免正则全文扫描
    QString modifyToolJson = R"({
        "name": "modify_article",
        "description": "Trigger when user wants to modify the article. Prefer specifying section_id when the user targets a specific section.",
        "parameters": {
            "type": "object",
            "required": ["instruction"],
            "properties": {
                "instruction": {
                    "type": "string",
                    "description": "The specific instruction for modifying the article."
                },
                "section_id": {
                    "type": "string",
                    "description": "Optional. The ID of the specific section to modify (e.g. 's2_1'). If omitted, the entire article is considered."
                }
            }
        }
    })";

    QJsonDocument modifyDoc = QJsonDocument::fromJson(modifyToolJson.toUtf8());
    m_tools.append(modifyDoc.object());
}

ArticleAdjustAgent::~ArticleAdjustAgent()
{
}

bool ArticleAdjustAgent::initialize()
{
    m_modificationAgent = QSharedPointer<ArticleModificationAgent>::create(this);
    return true;
}

QSharedPointer<LlmAgent> ArticleAdjustAgent::create()
{
    return QSharedPointer<LlmAgent>(new ArticleAdjustAgent());
}

QJsonObject ArticleAdjustAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    // 从 WritingState 读取文章内容（取代原来从 params["article"] 直接取字符串）
    WritingState state = WritingState::fromParams(params);
    m_article = state.articleContent();

    // 构建 ArticleModel，并将带 ID 注记的 Markdown 追加到 system prompt，
    // 使 LLM 能够在 modify_article 工具调用中精确指定 section_id
    m_articleModel = ArticleModel::fromMarkdown(m_article);
    if (!m_articleModel.isEmpty()) {
        m_systemPrompt = m_systemPrompt + "\n\n# Current Article Structure (with Section IDs)\n"
                         + "Use the [ID: xxx] markers below when calling modify_article with section_id.\n\n"
                         + m_articleModel.toAnnotatedMarkdown();
    }

    QJsonObject response = LlmAgent::processRequest(question, history, params);

    QString newArticle;
    QJsonArray contextArray = response.value("context").toArray();
    for (auto context : contextArray) {
        QJsonObject contextObj = context.toObject();
        if (contextObj.value("role") == "tool") {
            newArticle = contextObj.value("content").toString() + "\n\n";
            break;
        }
    }
    QString title;
    QRegularExpression re(R"(^#\s+(.*)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = re.match(newArticle);
    if (match.hasMatch()) {
        title = match.captured(1).trimmed();
    } else {
        title = tr("Untitled");
    }

    if (!newArticle.isEmpty()) {
        QJsonObject message;
        QJsonObject contentObj;
        contentObj.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
        contentObj.insert("title", title);
        contentObj.insert("content", newArticle);
        message.insert("content", contentObj);
        message.insert("chatType", ChatAction::ChatDocCard);
        QJsonObject wrapper;
        wrapper.insert("message", message);
        wrapper.insert("stream", true);
        emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
    }

    return response;
}

QPair<int, QString> ArticleAdjustAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName == "modify_article") {
        QString instruction = params.value("instruction").toString();
        QString sectionId   = params.value("section_id").toString();

        if (m_article.isEmpty()) {
            return qMakePair(0, QString("Error: No article content found to modify."));
        }

        m_modificationAgent->setModel(m_llm);
        QString modifiedArticle;

        if (!sectionId.isEmpty() && !m_articleModel.isEmpty()) {
            // 精确模式：仅取出目标章节，交给 LLM 重写，再写回 ArticleModel
            const ArticleModel::Section *sec = m_articleModel.sectionById(sectionId);
            if (sec) {
                if (!sec->content.isEmpty()) {
                    // 精确模式：章节有直接内容，只重写本节正文
                    QString heading(sec->level, '#');
                    QString sectionText = QString("%1 %2\n\n%3").arg(heading, sec->title, sec->content);
                    QString rewritten = m_modificationAgent->modifyArticle(sectionText, instruction);

                    if (rewritten.trimmed() == "DELETE_SECTION") {
                        m_articleModel.deleteSection(sectionId);
                    } else {
                        // 解析重写结果：若首行为标题行则同时更新 title 和 content，
                        // 避免旧 title 残留导致渲染出两个标题
                        static QRegularExpression headRe(R"(^#{1,6}\s+(.+))");
                        QRegularExpressionMatch hm = headRe.match(rewritten.trimmed());
                        if (hm.hasMatch()) {
                            m_articleModel.updateSectionTitle(sectionId, hm.captured(1).trimmed());
                            int newlinePos = rewritten.indexOf('\n');
                            QString newContent = (newlinePos != -1)
                                                     ? rewritten.mid(newlinePos + 1).trimmed()
                                                     : QString();
                            m_articleModel.updateSection(sectionId, newContent);
                        } else {
                            m_articleModel.updateSection(sectionId, rewritten.trimmed());
                        }
                    }
                    modifiedArticle = m_articleModel.toMarkdown();
                } else {
                    // 章节无直接内容（内容在子节中）：从原始文章按位置提取完整块
                    // 找该标题行在 m_article 中的位置
                    QRegularExpression headFindRe(
                        QString(R"(^#{%1}\s+%2[^\n]*$)")
                            .arg(sec->level)
                            .arg(QRegularExpression::escape(sec->title)),
                        QRegularExpression::MultilineOption);
                    QRegularExpressionMatch headMatch = headFindRe.match(m_article);

                    if (headMatch.hasMatch()) {
                        int blockStart = headMatch.capturedStart();
                        // 找下一个同级或更高级标题（块的结束位置）
                        QRegularExpression blockEndRe(
                            QString(R"(^#{1,%1}\s+)").arg(sec->level),
                            QRegularExpression::MultilineOption);
                        QRegularExpressionMatch endMatch = blockEndRe.match(m_article, headMatch.capturedEnd());
                        int blockEnd = endMatch.hasMatch() ? endMatch.capturedStart() : m_article.length();

                        QString sectionBlock = m_article.mid(blockStart, blockEnd - blockStart).trimmed();
                        QString rewritten = m_modificationAgent->modifyArticle(sectionBlock, instruction);

                        modifiedArticle = m_article;
                        if (rewritten.trimmed() == "DELETE_SECTION") {
                            modifiedArticle.remove(blockStart, blockEnd - blockStart);
                        } else {
                            modifiedArticle.replace(blockStart, blockEnd - blockStart,
                                                    rewritten.trimmed() + "\n\n");
                        }
                    } else {
                        // 找不到标题，降级为全文修改
                        modifiedArticle = m_modificationAgent->modifyArticle(m_article, instruction);
                    }
                }
            } else {
                // section_id 未找到，降级为全文修改
                modifiedArticle = m_modificationAgent->modifyArticle(m_article, instruction);
            }
        } else {
            // 全文模式（无 section_id 或 ArticleModel 为空）
            modifiedArticle = m_modificationAgent->modifyArticle(m_article, instruction);
        }

        if (!modifiedArticle.isEmpty()) {
            m_article = modifiedArticle; // 更新缓存供同一会话多次调用
            return qMakePair(0, modifiedArticle);
        } else {
            return qMakePair(-1, QString("Error: Failed to modify article."));
        }
    }

    return qMakePair(-1, QString("Unknown tool: %1").arg(toolName));
}

QString ArticleAdjustAgent::removeReferenceTags(const QString &content)
{
    QString result = content;
    QRegularExpression re(R"(<reference>.*?</reference>)", QRegularExpression::DotMatchesEverythingOption);
    result.replace(re, "");
    return result;
}

} // namespace uos_ai
