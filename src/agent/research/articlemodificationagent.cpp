#include "articlemodificationagent.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>
#include <functional>
#include <QMap>

namespace uos_ai {

ArticleModificationAgent::ArticleModificationAgent(QObject *parent) : LlmAgent(parent)
{
    m_name = "ArticleModificationAgent";
    m_description = "Agent specific for modifying markdown articles based on instructions.";
    m_systemPrompt = "You are an Article Modification Agent."; // Basic prompt, specific prompts used in functions
}

ArticleModificationAgent::~ArticleModificationAgent()
{
}

QSharedPointer<LlmAgent> ArticleModificationAgent::create()
{
    return QSharedPointer<LlmAgent>(new ArticleModificationAgent());
}

// Helper to calculate logical end position of a section (including its children)
// REMOVED: getLogicalEndPos is no longer needed as endPos is pre-calculated in parseSections.

QString ArticleModificationAgent::modifyArticle(const QString &articleContent, const QString &instruction)
{
    QList<Section> sections = parseSections(articleContent);
    if (sections.isEmpty()) {
        return rewriteContent(articleContent, instruction);
    }

    QStringList targetHeaders = determineModificationScope(sections, instruction);

    if (targetHeaders.contains("ALL") || targetHeaders.isEmpty()) {
         return rewriteContent(articleContent, instruction);
    }

    // We need to update specific sections.
    // 1. Map target headers to section indices.
    QList<int> indicesToModify;
    for (const QString &header : targetHeaders) {
        bool found = false;
        // Exact match
        for (int i = 0; i < sections.size(); ++i) {
            if (sections[i].headerTitle == header) {
                indicesToModify.append(i);
                found = true;
                break; 
            }
        }
        // Fuzzy match if exact match fails
        if (!found) {
            QString cleanHeader = header.simplified();
            cleanHeader.remove(QRegularExpression(R"([^\w\u4e00-\u9fa5])")); // Keep words and Chinese chars
            
            for (int i = 0; i < sections.size(); ++i) {
                QString cleanSectionTitle = sections[i].headerTitle.simplified();
                cleanSectionTitle.remove(QRegularExpression(R"([^\w\u4e00-\u9fa5])"));
                
                if (!cleanHeader.isEmpty() && !cleanSectionTitle.isEmpty() && 
                   (cleanSectionTitle == cleanHeader || cleanSectionTitle.contains(cleanHeader) || cleanHeader.contains(cleanSectionTitle))) {
                    indicesToModify.append(i);
                    found = true;
                    break;
                }
            }
        }
    }
    
    if (indicesToModify.isEmpty()) {
         return rewriteContent(articleContent, instruction);
    }

    // 2. Filter out redundant children (if Chapter 1 is selected, ignore Chapter 1.1)
    // Since endPos is now in Section struct, we don't need a separate map.

    QList<int> finalIndices;
    std::sort(indicesToModify.begin(), indicesToModify.end());

    for (int i : indicesToModify) {
        bool isChild = false;
        int startI = sections[i].startPos;
        
        for (int parent : finalIndices) {
            int startP = sections[parent].startPos;
            int endP = sections[parent].endPos;
            
            if (startI > startP && startI < endP) {
                isChild = true;
                break;
            }
        }
        if (!isChild) {
            finalIndices.append(i);
        }
    }

    // 3. Apply modifications from Bottom to Top
    std::sort(finalIndices.begin(), finalIndices.end(), std::greater<int>());

    QString modifiedArticle = articleContent;

    for (int idx : finalIndices) {
        int start = sections[idx].startPos;
        int end = sections[idx].endPos;
        
        QString originalText = modifiedArticle.mid(start, end - start);
        QString newText = rewriteContent(originalText, instruction);
        
        if (newText.trimmed() == "DELETE_SECTION") {
            modifiedArticle.remove(start, end - start);
        } else {
            modifiedArticle.replace(start, end - start, newText);
        }
    }

    return modifiedArticle;
}

QList<ArticleModificationAgent::Section> ArticleModificationAgent::parseSections(const QString &article)
{
    QList<Section> sections;
    // Allow leading whitespace
    QRegularExpression re(R"(^\s*(#{1,6})\s+(.*)$)", QRegularExpression::MultilineOption);
    
    QRegularExpressionMatchIterator i = re.globalMatch(article);
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        Section sec;
        sec.level = match.captured(1).length();
        sec.headerTitle = match.captured(2).trimmed();
        sec.startPos = match.capturedStart(); 
        sec.endPos = -1; // Placeholder
        sections.append(sec);
    }

    // Pre-calculate logical endPos for all sections
    int totalLen = article.length();
    for (int i = 0; i < sections.size(); ++i) {
        int currentLevel = sections[i].level;
        int end = totalLen;
        for (int j = i + 1; j < sections.size(); ++j) {
            if (sections[j].level <= currentLevel) {
                end = sections[j].startPos;
                break;
            }
        }
        sections[i].endPos = end;
    }

    return sections;
}

QStringList ArticleModificationAgent::determineModificationScope(const QList<Section> &sections, const QString &instruction)
{
    QString outline;
    for (const auto &sec : sections) {
        outline += QString("%1 %2\n").arg(QString(sec.level, '#'), sec.headerTitle);
    }
    
    QString prompt = QString(R"(You are an intelligent editor.
Instruction: "%1"

Article Outline:
%2

Task: Identify which sections need to be modified based on the instruction.
Rules:
1. If the instruction applies to the whole article (e.g. "change tone", "fix formatting globally"), return ["ALL"].
2. If the instruction applies to specific section(s), return their exact Header Titles in a JSON list.
3. If a modification to a Chapter is requested, select the Chapter Title (this implies including its subsections).
4. Only select the highest-level necessary header.
5. STRICTLY return a JSON array of strings. NO other text.

Example 1:
Instruction: "Rewrite the Conclusion."
Outline:
# Introduction
# Conclusion
Output: ["Conclusion"]

Example 2:
Instruction: "Delete the section on Market Analysis."
Outline:
# Introduction
# Market Analysis
## Competitors
# Conclusion
Output: ["Market Analysis"]

Output:
)")
    .arg(instruction, outline);

    QJsonObject req;
    req["role"] = "user";
    req["content"] = prompt;

    QJsonObject response = LlmAgent::processRequest(req, QJsonArray(), QVariantHash());
    QString content = response.value("content").toString();
    
    QRegularExpression re(R"(\[.*\])", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = re.match(content);
    QStringList results;
    
    if (match.hasMatch()) {
        QJsonDocument doc = QJsonDocument::fromJson(match.captured(0).toUtf8());
        QJsonArray arr = doc.array();
        for (auto val : arr) {
            results.append(val.toString());
        }
    } else {
        if (content.contains("ALL")) return {"ALL"};
    }
    return results;
}

QString ArticleModificationAgent::rewriteContent(const QString &originalContent, const QString &instruction)
{
    QString prompt = QString(R"(You are an expert editor.
Instruction: "%1"

Original Text:
%2

Task: Rewrite the Original Text to satisfy the Instruction.
Requirements:
1. If the instruction asks to DELETE or REMOVE the section entirely, output exactly: DELETE_SECTION
2. Otherwise, maintain the original Markdown structure (headers, lists) unless the instruction specifically asks to change structure.
3. Output ONLY the modified text. No conversational fillers.

Example 1:
Instruction: "Fix typos."
Original: "# Intro\nThsi is a tst."
Output: "# Intro\nThis is a test."

Example 2:
Instruction: "Delete this section."
Original: "# Chapter 1\nContent..."
Output: DELETE_SECTION

Output:
)")
    .arg(instruction, originalContent);

    QJsonObject req;
    req["role"] = "user";
    req["content"] = prompt;
    
    QJsonObject response = LlmAgent::processRequest(req, QJsonArray(), QVariantHash());
    QString resText = response.value("content").toString();
    
    // Handle markdown code blocks using regex to catch variations like ```markdown, ``` markdown, etc.
    QRegularExpression re(R"(^```(markdown)?\s*)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(resText);
    
    if (match.hasMatch()) {
        resText.remove(0, match.capturedLength());
        if (resText.endsWith("```")) {
            resText.chop(3);
        }
    } else if (resText.startsWith("```")) {
        // Fallback for standard code block if regex didn't catch (unlikely given the regex)
         resText = resText.mid(3);
         if (resText.endsWith("```")) resText.chop(3);
    }
    
    return resText.trimmed();
}

} // namespace uos_ai
