#ifndef ARTICLEADJUSTAGENT_H
#define ARTICLEADJUSTAGENT_H

#include "llmagent.h"
#include "articlemodel.h"

namespace uos_ai {

class ArticleAdjustAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ArticleAdjustAgent(QObject *parent = nullptr);
    virtual ~ArticleAdjustAgent() override;

    static QSharedPointer<LlmAgent> create();

    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params = {}) override;

protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    QString      m_article;       // original article, used by callTool to apply section patches
    ArticleModel m_articleModel;  // parsed model for section-level updates
    QString      m_pendingArticle;
    QString      m_articleId;     // article id from stage params, used by callTool
};

} // namespace uos_ai

#endif // ARTICLEADJUSTAGENT_H
