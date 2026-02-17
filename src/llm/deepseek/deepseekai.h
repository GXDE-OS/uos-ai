#ifndef DEEPSEEKAI_H
#define DEEPSEEKAI_H

#include "llm.h"
#include "uosai_global.h"

namespace uos_ai {

class DeepSeekAI : public LLM
{
    Q_OBJECT
public:
    inline virtual QString baseUrl() {
        return QString("https://api.deepseek.com/chat/completions");
    }

    inline virtual QString modelId() {
        return QString("deepseek-reasoner");
    }

    explicit DeepSeekAI(const LLMServerProxy &serverproxy);

    QJsonObject predict(const QString &content, const QJsonArray &functions) override;
    QPair<int, QString> verify() override;

protected slots:
    void onReadyReadChatDeltaContent(const QByteArray &content);
};
}
#endif // DEEPSEEKAI_H
