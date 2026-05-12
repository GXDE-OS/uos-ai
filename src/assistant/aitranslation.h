#ifndef AITRANSLATION_H
#define AITRANSLATION_H

#include "abstractassistant.h"

namespace uos_ai {

class AITranslation : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit AITranslation(QObject *parent = nullptr);
    ~AITranslation() override;

    void cancel() override;

    /** 返回翻译助手 FAQ 列表（JSON 字符串） */
    static QString getTranslationFAQ();

Q_SIGNALS:
    void requestCancel();
protected:
    //翻译不需要上下文
    void processMessage(ModelMessage &currentMessage, bool retry);
protected:
    QVariantHash run() override;
};

} // namespace uos_ai

#endif // AITRANSLATION_H
