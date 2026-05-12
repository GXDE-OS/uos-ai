#ifndef TRANSLATIONAGENT_H
#define TRANSLATIONAGENT_H

#include "../llmagent.h"

#include <QObject>
#include <QString>

namespace uos_ai {

class TranslationAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit TranslationAgent(QObject *parent = nullptr);
    ~TranslationAgent() override;

    /**
     * 初始化智能体
     * @returns {bool} 是否成功
     */
    bool initialize() override;

protected:
    /**
     * 获取系统提示词
     * @returns {QString} 系统提示词内容
     */
    QString systemPrompt() const override;

    /**
     * 调用工具
     * 执行指定的工具调用，传递相应的参数
     * @param {QString} toolName - 要调用的工具名称
     * @param {QJsonObject} params - 工具调用所需的参数
     * @returns {QPair<int, QString>} 工具调用的结果，包含状态码和返回内容
     *          状态码：0表示成功，非0表示错误
     *          QString：工具调用的返回内容或错误信息
     */
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

    /**
      * 初始化工具列表
      */
     void createTools();
};

} // namespace uos_ai

#endif // TRANSLATIONAGENT_H