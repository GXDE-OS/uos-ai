#ifndef PPTLLM_H
#define PPTLLM_H

#include <llmmodel.h>

namespace uos_ai {

class PPTLLM : public QObject, public LLMModel
{
    Q_OBJECT
public:
    static inline QString modelID() {
        return QString("AI PPT");
    }

    explicit PPTLLM();
    QString model() const override;

    //request API
    /**
    * @brief generate
    * @param content 发送问题内容
    * @param params 请求参数 {uos_ai::LLMModel::GENERATE_PARAM_TEMPERATURE\GENERATE_PARAM_FUNCTION}
    * @param stream  流式传输接口
    */
    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;
    QString token = "";

    // UOS AI 生成回答中，点击停止会调用此接口
    void setAbort() override;
private:
    QString getContent(int id,  streamFuncion stream = nullptr, void *user = nullptr);
    QString removefirstLine(const QString &text);

signals:
    void sigAbort();
};

}

#endif // PPTLLM_H
