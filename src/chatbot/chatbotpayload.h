#pragma once

#include <QJsonObject>
#include <QString>

namespace uos_ai {
namespace chatbot {

/**
 * @brief ParsedPayload - IM 消息 payload 的类型化字段
 *
 * 统一解析 ChannelManager 转发的 QJsonObject，消除各模块中重复的字段提取代码。
 * memKey() 返回供 ChatMemory 使用的会话标识：私聊为 platform:convId，
 * 群聊为 platform:convId:senderId（按发送者隔离）。
 */
struct ParsedPayload {
    QString platform;
    QString convType;   // "user" | "group" | "guild"
    QString convId;
    QString senderId;
    QString replyTo;    // 私聊 = senderId，群聊 = convId
    QString content;    // message text (trimmed)

    static ParsedPayload from(const QJsonObject &payload);

    /** 返回 ChatMemory key：群聊附加 senderId 实现隔离 */
    QString memKey() const;
};

} // namespace chatbot
} // namespace uos_ai
