#ifndef ZHUPUAI_H
#define ZHUPUAI_H

#include "llm.h"

class ZhiPuAI : public LLM
{
public:
    ZhiPuAI(const LLMServerProxy &serverproxy);

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

private slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
};

#endif // ZHUPUAI_H
