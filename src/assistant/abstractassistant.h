#ifndef ABSTRACTASSISTANT_H
#define ABSTRACTASSISTANT_H

#include "conversation/messagenode.h"

#include <QObject>
#include <QSharedPointer>

namespace uos_ai {
class ConversationRecord;
class AbstractAssistant : public QObject
{
    Q_OBJECT
public:
    explicit AbstractAssistant(QObject *parent = nullptr);
    virtual ~AbstractAssistant();

    virtual void setModelId(const QString &modelId);
    virtual QString modelId() const;
    virtual void setConversation(const QSharedPointer<ConversationRecord> &conv);
    virtual QSharedPointer<ConversationRecord> conversation() const;

    virtual bool initialize(const QVariantHash &);

    virtual QVariantHash execute();

    virtual void cancel() = 0;

    virtual void invokeAction(const QJsonObject &action);

    virtual QVariantHash lastError() const;
Q_SIGNALS:
    void started();
    void finished(const QVariantHash &);
    void pushMessage(const QString &message);
protected:
    virtual QVariantHash run() = 0;
protected:
    QString m_modelId;
    QSharedPointer<ConversationRecord> m_conversation;
    QVariantHash m_error;
    QVariantHash m_parameters;
};

using AssistantPtr = QSharedPointer<AbstractAssistant>;

} // namespace uos_ai

#endif // ABSTRACTASSISTANT_H
