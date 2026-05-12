#ifndef OAIMESSAGEPROTOCOL_H
#define OAIMESSAGEPROTOCOL_H

#include "global_define.h"
#include "conversation/messagenode.h"
#include "modeltool.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QHash>

namespace uos_ai {

class OaiMessageProtocol
{
public:
    explicit OaiMessageProtocol();
    virtual ~OaiMessageProtocol();

    virtual void initMessage(const QList<ModelMessage> &messages);
    virtual void addMessage(const ModelMessage &msg);
    virtual void addTool(const ModelTool &tool);
    virtual void addTools(const QList<ModelTool> &tools);
    virtual QJsonObject params(const QVariantHash &args);
    virtual QJsonArray messages() const;
    virtual QJsonArray tools() const;

    virtual bool parseChunk(const QByteArray &chunkData, QVariantHash &content);
    virtual bool parseResponse(const QByteArray &data, QVariantHash &content);
    virtual void clearBuffer();
    virtual void setEnableReasoning(bool enable);
protected:
    virtual QJsonObject buildMessage(const ModelMessage &msg);
    virtual QJsonObject buildContent(const MetaMessage &msg);
    virtual QJsonObject buildToolCall(const ModelToolCall &toolCall);
    virtual QString convertContentType(ContentType contentType);
    QString sanitizeToolName(const QString &name);
    QString restoreToolName(const QString &name) const;
protected:
    QJsonArray m_messages;
    QJsonArray m_tools;
    QHash<QString, QString> m_toolNameMap;
    bool m_reasoning = false;

    QString m_buffer;
};

} // namespace uos_ai

#endif // OAIMESSAGEPROTOCOL_H
