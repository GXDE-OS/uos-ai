#ifndef ARTICLEMODIFICATIONAGENT_H
#define ARTICLEMODIFICATIONAGENT_H

#include "llmagent.h"

namespace uos_ai {

class ArticleModificationAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ArticleModificationAgent(QObject *parent = nullptr);
    virtual ~ArticleModificationAgent();

    static QSharedPointer<LlmAgent> create();
    
    // Main entry point to modify the article
    QString modifyArticle(const QString &articleContent, const QString &instruction);

private:
    struct Section {
        QString headerTitle = QString("");
        int level = 0;
        int startPos = 0; // Start of the header line
        int endPos = 0;   // End of the content (start of next section or end of file)
        QString fullText = QString(""); // Header line + content
    };

    // Helper to parse markdown structure
    QList<Section> parseSections(const QString &article);
    
    // Step 1: Determine scope (returns list of headers to modify, or "ALL")
    QStringList determineModificationScope(const QList<Section> &sections, const QString &instruction);
    
    // Step 2: Rewrite specific content
    QString rewriteContent(const QString &originalContent, const QString &instruction);
    
    static int getLogicalEndPos(const QList<Section> &sections, int index, int totalLength);
};

} // namespace uos_ai

#endif // ARTICLEMODIFICATIONAGENT_H
