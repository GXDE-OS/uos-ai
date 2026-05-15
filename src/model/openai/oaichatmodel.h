#ifndef OAICHATMODEL_H
#define OAICHATMODEL_H

#include "abstractchatmodel.h"

namespace uos_ai {
class OaiMessageProtocol;
class OaiChatModel : public AbstractChatModel
{
    Q_OBJECT
public:
    explicit OaiChatModel(QObject *parent = nullptr);
    ~OaiChatModel() override;

    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;

    virtual void setApiHost(const QString &host);
    virtual QString apiHost() const;
Q_SIGNALS:
    void requestCancel();
public Q_SLOTS:
    void cancel() override;
protected Q_SLOTS:
    void chunkReceived(const QByteArray &chunk);
protected:
    QString m_host;
    QSharedPointer<OaiMessageProtocol> m_protocol;
};

} // namespace uos_ai

#endif // OAICHATMODEL_H
