#ifndef GENERICAGENT_H
#define GENERICAGENT_H

#include "llmagent.h"
#include "toolresult.h"
#include "oscontrol/deepinabilitymanager.h"
#include "oscontrol/oscallcontext.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace uos_ai {

class GenericAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit GenericAgent(QObject *parent = nullptr);
    ~GenericAgent() override;

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

    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

    /**
      * 初始化工具列表
      */
     void createTools();

    /**
     * 处理工具调用结果中的卡片数据
     * 将卡片数据转换为前端可识别的格式并发送
     * @param {ToolCallResult} result - 工具调用结果
     */
    void handleCardData(const ToolCallResult &result);
};

} // namespace uos_ai

#endif // GENERICAGENT_H
