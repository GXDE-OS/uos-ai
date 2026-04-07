#include "chatbotpayload.h"

using namespace uos_ai::chatbot;

ParsedPayload ParsedPayload::from(const QJsonObject &payload)
{
    ParsedPayload p;
    p.platform  = payload.value("platform").toString();
    const QJsonObject conv = payload.value("conversation").toObject();
    p.convType  = conv.value("type").toString();
    p.convId    = conv.value("id").toString();
    p.senderId  = payload.value("sender").toObject().value("id").toString();
    p.replyTo   = (p.convType == QLatin1String("user")) ? p.senderId : p.convId;
    p.content   = payload.value("content").toObject().value("text").toString().trimmed();
    return p;
}

QString ParsedPayload::memKey() const
{
    const QString convKey       = platform + ":" + convId;
    const QString senderForMem  = (convType == QLatin1String("user")) ? QString() : senderId;
    return senderForMem.isEmpty() ? convKey : convKey + ":" + senderForMem;
}
