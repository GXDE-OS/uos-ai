#ifndef MESSAGENODE_H
#define MESSAGENODE_H

#include "global_define.h"

#include <QList>
#include <QVariant>
#include <QSharedPointer>
#include <QJsonObject>
#include <QVariantHash>
#include <QReadWriteLock>

namespace uos_ai {

enum MessageRole {
    MrVirtual = 0,
    MrUser = 1,
    MrAssistant = 2
};

struct MetaMessage
{
    ContentType type;
    QVariant data;

    static QList<MetaMessage> fromHash(const QVariantHash &content);
};

struct RenderMessage : MetaMessage
{
    QJsonObject toJson() const;

    static RenderMessage fromJson(QJsonObject obj);
    static RenderMessage createTool(const QString &id, const QString &name, const QString &params, int status, const QString &result = {});
};

// for model input/output
struct ModelMessage
{
    QString role; //system, user, assistant, tool
    QString source; // agent name,
    QList<MetaMessage> content;

    QJsonObject toJson() const;
    static ModelMessage fromJson(QJsonObject obj);
    static ModelMessage toolOutput(const QString &id, const QString &content);
};

class MessageNode
{
public:
    QJsonObject toJson() const;
    static QSharedPointer<MessageNode> fromJson(const QJsonObject &json);

    // Role
    MessageRole getRole() const;
    void setRole(const MessageRole &role);

    // Id
    QString getId() const;
    void setId(const QString &id);

    // ModelName
    QString getModelName() const;
    void setModelName(const QString &modelName);

    // ModelId
    QString getModelId() const;
    void setModelId(const QString &modelId);

    // Render
    QList<RenderMessage> getRender() const;
    void setRender(const QList<RenderMessage> &render);
    void appendRender(const RenderMessage &value);

    // Message
    QList<ModelMessage> getMessage() const;
    void setMessage(const QList<ModelMessage> &message);
    void appendMessage(const ModelMessage &value);

    // Previous
    QString getPrevious() const;
    void setPrevious(const QString &previous);

    // Next
    QStringList getNext() const;
    void setNext(const QStringList &next);
    void appendNext(const QString &value);

    // CurNext
    QString getCurNext() const;
    void setCurNext(const QString &curNext);

    // Extension
    QVariantHash getExtension() const;
    void setExtension(const QVariantHash &extension);

private:
    MessageRole role;
    QString id;
    QString modelName;
    QString modelId;
    QList<RenderMessage> render;
    QList<ModelMessage> message;

    QString previous;
    QStringList next;
    QString curNext;

    QVariantHash extension;

    mutable QReadWriteLock m_lock;
};


using MessageNodePtr = QSharedPointer<MessageNode>;
using MetaMessageList = QList<MetaMessage>;
using RenderMessageList = QList<RenderMessage>;
}

Q_DECLARE_METATYPE(uos_ai::MetaMessageList)
Q_DECLARE_METATYPE(uos_ai::RenderMessageList)
Q_DECLARE_METATYPE(uos_ai::ModelMessage)

#endif // MESSAGENODE_H
