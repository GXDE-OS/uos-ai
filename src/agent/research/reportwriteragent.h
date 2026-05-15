#ifndef REPORTWRITERAGENT_H
#define REPORTWRITERAGENT_H

#include "llmagent.h"
#include "referencemanager.h"

namespace uos_ai {

class ReportWriterAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ReportWriterAgent(QObject *parent = nullptr);

    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params = {}) override;

private:
    struct SectionData {
        QString title;
        QString outline;
        QStringList selectedRefIds;
        QString retrievedContent;
        QString filteredMaterialSection;
    };

    // processRequest sub-steps
    QList<SectionData> prepareSections(const QJsonObject &outlineObj, const QString &materialSection);
    void loadReferences(const QVariantHash &params);
    QSharedPointer<AbstractChatModel> createSelectModel();
    QString buildSectionPrompt(bool isFirst, const QString &materialSection,
                               const QString &fullOutline, const QString &context,
                               const QString &sectionOutline, const QString &retrievedContent);
    ModelMessage buildMaskedHistoryMessage(bool isFirst, const QString &materialSection,
                                          const QString &fullOutline, const QString &context,
                                          const QString &sectionOutline);
    static void appendMaterialPrompt(QString &prompt, const QString &retrievedContent);
    void ensureSectionSeparator();
    void finalizeReport(const QString &title, const QString &context, const QString &articleId, QVariantHash &result);

    // Emit helpers
    QString emitSummary(const QString &context, const QString &article);
    QList<QStringList> selectRefsForAllSections(const QStringList &sectionOutlines, const QString &materialSection,
                                                QSharedPointer<AbstractChatModel> selectModel);
    void trimConversationHistory(QList<ModelMessage> &history, int currentIndex);

    ReferenceManager m_refManager;
    QString m_report;
};

} // namespace uos_ai

#endif // REPORTWRITERAGENT_H
