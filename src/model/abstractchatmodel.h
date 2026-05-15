#ifndef ABSTRACTCHATMODEL_H
#define ABSTRACTCHATMODEL_H

#include "abstractmodel.h"
#include "conversation/messagenode.h"

#include <QObject>
#include <QVariantHash>

namespace uos_ai {

class AbstractChatModel : public AbstractModel
{
    Q_OBJECT
public:
    explicit AbstractChatModel(QObject *parent = nullptr);
    ~AbstractChatModel() override;
    virtual QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) = 0;
public Q_SLOTS:
    virtual void cancel() = 0;
Q_SIGNALS:
    void messageReceived(const MetaMessageList &msgs);
};

} // namespace uos_ai

#endif // ABSTRACTCHATMODEL_H
