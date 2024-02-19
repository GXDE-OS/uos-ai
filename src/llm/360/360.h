#ifndef GPT360_H
#define GPT360_H

#include "llm.h"

class Gpt360 : public LLM
{
public:
    Gpt360(const LLMServerProxy &serverproxy);

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
    void onReadyReadChatDeltaContent(const QByteArray &content);
};

#endif // GPT360_H
