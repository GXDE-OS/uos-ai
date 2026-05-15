#include "chatbotpayload.h"
#include "chatbot_key_define.h"
#include "global_key_define.h"

using namespace uos_ai;
using namespace uos_ai::chatbot;

ParsedPayload ParsedPayload::from(const QJsonObject &payload)
{
    ParsedPayload p;
    p.platform  = payload.value(STR_KEY_PLATFORM).toString();
    const QJsonObject conv = payload.value(STR_KEY_CONVERSATION).toObject();
    const QJsonObject content = payload.value(STR_KEY_CONTENT).toObject();
    p.convType  = conv.value(STR_KEY_TYPE).toString();
    p.convId    = conv.value(STR_KEY_ID).toString();
    p.senderId  = payload.value(STR_KEY_SENDER).toObject().value(STR_KEY_ID).toString();
    p.replyTo   = (p.convType == QLatin1String(STR_KEY_USER)) ? p.senderId : p.convId;
    p.content   = content.value(STR_KEY_TEXT).toString().trimmed();
    p.contentSource = content.value(STR_KEY_SOURCE).toString();
    p.meta      = payload.value(STR_KEY_META).toObject();
    return p;
}

QString ParsedPayload::memKey() const
{
    const QString convKey       = platform + ":" + convId;
    const QString senderForMem  = (convType == QLatin1String("user")) ? QString() : senderId;
    return senderForMem.isEmpty() ? convKey : convKey + ":" + senderForMem;
}
