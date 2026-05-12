#include "genericagent.h"
#include "toolregistry.h"
#include "toolhandler.h"
#include "conversation/messagenode.h"

#include <QDir>
#include <QDate>
#include <QLoggingCategory>
#include <QCoreApplication>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

GenericAgent::GenericAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "GenericAgent";
    m_description = "A Generic Agent for UOS AI";
    m_systemPrompt = R"(You are UOS AI, an intelligent AI assistant integrated into the Deepin Desktop Environment (DDE) on UOS (an operating system based on Debian Linux).
    m_systemPrompt = R"(You are %1, an intelligent AI assistant integrated into the Deepin Desktop Environment (DDE) on UOS (an operating system based on Debian Linux).
You have tools at your disposal to solve user's task. Only calls tools when they are necessary. If the USER's task is general or you already know the answer, just respond without calling tools.
Today is %2.)";
}

GenericAgent::~GenericAgent()
{
}

bool GenericAgent::initialize()
{
    createTools();
    return LlmAgent::initialize();
}

QString GenericAgent::systemPrompt() const
{
    return m_systemPrompt
            .arg(QCoreApplication::translate("uos_ai::AssistantManager", "UOS AI"))
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd ddd"));
}

void GenericAgent::createTools()
{
    m_tools = ToolRegistry::getAllTools();
}

QPair<int, QString> GenericAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    // 调用 ToolHandler 处理工具调用
    ToolCallResult result = ToolHandler::callTool(toolName, params);

    // 处理工具调用结果中的卡片数据
    handleCardData(result);

    QString message = result.message;
    if (result.cardType != CardType::None) {
        message = "[CARD_DISPLAYED] A UI card has been displayed to the user. Do NOT explain or describe the card. Just give a very brief acknowledgment or respond with empty content.";
    }

    return qMakePair(result.errorCode, message);
}

void GenericAgent::handleCardData(const ToolCallResult &result)
{
    // 如果工具调用返回了卡片数据，发送command_card类型的RenderMessage到前端
    if (result.cardType != CardType::None) {
        RenderMessage cardMsg;
        cardMsg.type = ContentType::CntCommandCard;

        QVariantHash cardDataHash;

        QString cardTypeStr;
        switch (result.cardType) {
            case CardType::SwitchCard:
                cardTypeStr = "switch_card";
                break;
            case CardType::SliderCard:
                cardTypeStr = "slider_card";
                break;
            case CardType::AppStoreCard:
                cardTypeStr = "app_store_card";
                break;
            case CardType::ScheduleCard:
                cardTypeStr = "schedule_card";
                break;
            default:
                cardTypeStr = "none";
                break;
        }
        cardDataHash["cardType"] = cardTypeStr;
        qCInfo(logAgent) << "GenericAgent creating command_card, cardType:" << static_cast<int>(result.cardType)
                         << "cardTypeStr:" << cardTypeStr;

        // 将cardData的QJsonObject转换为QVariantHash
        QVariantHash cardDataVariant = result.cardData.toVariantHash();
        cardDataHash["cardData"] = cardDataVariant;
        cardDataHash["toolName"] = result.toolName;
        cardDataHash["message"] = result.message;
        cardDataHash["errorCode"] = result.errorCode;

        if (!result.extraData.isEmpty()) {
            QVariantHash extraDataVariant = result.extraData.toVariantHash();
            cardDataHash["extraData"] = extraDataVariant;
        }

        cardMsg.data = cardDataHash;

        RenderMessageList cardMsgs;
        cardMsgs.append(cardMsg);
        emit messageReceived(cardMsgs);
    }
}
