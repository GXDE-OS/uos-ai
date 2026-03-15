#ifndef ARTICLEADJUSTAGENT_H
#define ARTICLEADJUSTAGENT_H

#include "llmagent.h"
#include "articlemodel.h"

namespace uos_ai {
class ArticleModificationAgent;
class ArticleAdjustAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ArticleAdjustAgent(QObject *parent = nullptr);
    virtual ~ArticleAdjustAgent() override;

    bool initialize() override;
    static QSharedPointer<LlmAgent> create();

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;


protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    QString       m_article;         // 当前文章原始 Markdown
    ArticleModel  m_articleModel;    // 结构化文章模型（带 Section ID）
    QSharedPointer<ArticleModificationAgent> m_modificationAgent;

    QString removeReferenceTags(const QString &content);
};

} // namespace uos_ai

#endif // ARTICLEADJUSTAGENT_H
