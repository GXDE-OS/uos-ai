#ifndef TOOLRESULT_H
#define TOOLRESULT_H

#include <QString>
#include <QJsonObject>
#include <QVariant>

namespace uos_ai {

/**
 * @brief 指令卡片类型枚举
 */
enum class CardType {
    None,         // 无卡片，直接输出结果
    SwitchCard,   // 开关卡片（用于蓝牙、WiFi、勿扰模式等）
    SliderCard,   // 滑块卡片（用于屏幕亮度、音量等）
    AppStoreCard, // 应用商店卡片（用于应用商店搜索结果）
    ScheduleCard  // 日程卡片
};

/**
 * @brief 工具调用结果结构体
 */
struct ToolCallResult {
    int errorCode;              // 错误码(0表示成功)
    QString message;            // 结果消息
    QString toolName;           // 工具名称
    CardType cardType;          // 卡片类型
    QJsonObject cardData;       // 卡片数据（用于前端渲染）
    QJsonObject extraData;      // 额外数据（如图标、应用名称等）

    ToolCallResult()
        : errorCode(-1)
        , cardType(CardType::None)
    {}

    ToolCallResult(int code, const QString &msg)
        : errorCode(code)
        , message(msg)
        , cardType(CardType::None)
    {}

    /**
     * @brief 判断是否成功
     */
    bool isSuccess() const {
        return errorCode == 0;
    }

    /**
     * @brief 转换为JSON对象
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["errorCode"] = errorCode;
        obj["message"] = message;
        obj["toolName"] = toolName;
        obj["cardType"] = static_cast<int>(cardType);
        obj["cardData"] = cardData;
        obj["extraData"] = extraData;
        return obj;
    }

    /**
     * @brief 从JSON对象创建
     */
    static ToolCallResult fromJson(const QJsonObject &obj) {
        ToolCallResult result;
        result.errorCode = obj["errorCode"].toInt();
        result.message = obj["message"].toString();
        result.toolName = obj["toolName"].toString();
        result.cardType = static_cast<CardType>(obj["cardType"].toInt());
        result.cardData = obj["cardData"].toObject();
        result.extraData = obj["extraData"].toObject();
        return result;
    }
};

} // namespace uos_ai

#endif // TOOLRESULT_H
