#ifndef POSTERLLM_H
#define POSTERLLM_H

#include <llmmodel.h>

namespace uos_ai {

class PosterLLM : public QObject, public LLMModel
{
    Q_OBJECT
public:
    static inline QString modelID() {
        return QString("亦心 AI");
    }

    explicit PosterLLM();
    QString model() const override;

    //request API
    /**
    * @brief generate
    * @param content 发送问题内容
    * @param params 请求参数 {uos_ai::LLMModel::GENERATE_PARAM_TEMPERATURE\GENERATE_PARAM_FUNCTION}
    * @param stream  流式传输接口
    */
    QJsonObject generate(const QString &content, const QVariantHash &params, streamFuncion stream = nullptr, void *user = nullptr) override;

    // UOS AI 生成回答中，点击停止会调用此接口
    void setAbort() override;
private:
    QString getContent(QJsonObject object,  streamFuncion stream = nullptr, void *user = nullptr);
    QString key = "";

signals:
    void sigAbort();
};

}

#endif // POSTERLLM_H
