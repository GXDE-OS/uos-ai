#ifndef SPARKDESK_H
#define SPARKDESK_H

#include "llm.h"

class SparkDesk : public LLM
{
public:
    SparkDesk(const LLMServerProxy &serverproxy);

public:
    /**
     * @brief generateChatText
     * @param conversation
     * @param temperature
     * @return
     */
    QJsonObject predict(const QString &conversation, const QJsonArray &functions, const QString &systemRole = "", qreal temperature = 1.0) override;
    /**
     * @brief text2Image
     * @param prompt
     * @return
     */
    QList<QByteArray> text2Image(const QString &prompt, int number) override;

    /**
     * @brief verify
     * @return
     */
    QPair<int, QString> verify() override;

private slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
};

#endif // SPARKDESK_H
