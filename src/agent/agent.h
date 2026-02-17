#ifndef AGENT_H
#define AGENT_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantHash>

namespace uos_ai {

/**
 * 智能体基类，定义智能体的基本属性和接口
 */
class Agent
{
public:
    explicit Agent();
    virtual ~Agent();

    /**
     * 处理用户请求
     * @param {QJsonObject} question - 当前请求内容
     * @param {QJsonArray} messages - 聊天消息记录
     * @param {QVariantHash} params - 扩展参数
     * @returns {QJsonObject} 智能体工作流输出的消息记录
     */
    virtual QJsonObject processRequest(const QJsonObject &question, const QJsonArray &messages, const QVariantHash &params = {}) = 0;

    /**
     * 获取智能体名称
     * @returns {QString} 智能体名称
     */
    QString name() const;

    /**
     * 获取智能体描述
     * @returns {QString} 智能体描述
     */
    QString description() const;

    /**
     * 获取系统提示词
     * @returns {QString} 系统提示词
     */
    virtual QString systemPrompt() const;
protected:
    QString m_name;           // 智能体名称
    QString m_description;    // 智能体描述
    QString m_systemPrompt;   // 系统提示词
    QJsonArray m_tools;       // 智能体工具
};

} // namespace uos_ai

#endif // AGENT_H 
