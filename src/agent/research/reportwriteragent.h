#ifndef REPORTWRITERAGENT_H
#define REPORTWRITERAGENT_H

#include "agent/llmagent.h"

namespace uos_ai {

class ReportWriterAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ReportWriterAgent(QObject *parent = nullptr);

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &message, const QVariantHash &params = {}) override;

private:
    void outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown);
    void docCardContent(const QString &title, const QString &report);
    void sendReference(const QJsonObject &reference);

    QString m_streamBuffer;
};

} // namespace uos_ai

#endif // REPORTWRITERAGENT_H
