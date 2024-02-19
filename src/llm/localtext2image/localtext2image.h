#ifndef LOCALTEXT2IMAGE_H
#define LOCALTEXT2IMAGE_H

#include "llm.h"

#include <QSharedPointer>

class LocalText2Image : public LLM
{
    Q_OBJECT

public:
    LocalText2Image(const LLMServerProxy &serverproxy);

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
};

#endif // LOCALTEXT2IMAGE_H
