#ifndef CHATGPT_H
#define CHATGPT_H

#include "llm.h"

#include <QSharedPointer>

class TasManager;

class ChatGpt : public LLM
{
public:
    ChatGpt(const LLMServerProxy &serverproxy);

public:
    /**
     * @brief predict
     * @param content
     * @param systemRole
     * @param temperature
     * @return
     */
    QJsonObject predict(const QString &content, const QJsonArray &functions, const QString &systemRole = "", qreal temperature = 1.0) override;

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

private:
    /**
     * @brief modelId
     * @return
     */
    QString modelId();

private slots:
    /**
     * @brief onReadyReadChatDeltaContent
     * @param content
     */
    void onReadyReadChatDeltaContent(const QByteArray &content);

};

#endif // CHATGPT_H
