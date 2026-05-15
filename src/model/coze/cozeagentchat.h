#ifndef COZEAGENTCHAT_H
#define COZEAGENTCHAT_H

#include "abstractchatmodel.h"

namespace uos_ai {

class CozeAgentChat : public AbstractChatModel
{
    Q_OBJECT
public:
    explicit CozeAgentChat(QObject *parent = nullptr);
    ~CozeAgentChat() override;

    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;

    void setApiHost(const QString &host);
    QString apiHost() const;
    void setBotId(const QString &botId);
    QString botId() const;
Q_SIGNALS:
    void requestCancel();
public Q_SLOTS:
    void cancel() override;
protected Q_SLOTS:
    void chunkReceived(const QByteArray &chunk);
protected:
    QString generateJWTToken();
    QString getUniqueIdentifier();
    QPair<int, QString> ensureToken(QString &token);
    QJsonArray buildMessages(const QList<ModelMessage> &messages);
    QString parseContentString(const QByteArray &content);
protected:
    QString m_host;
    QString m_botId;
    QByteArray m_deltaContent;
    QString m_answerContent;
    QPair<int, QString> m_chatFailed;
};

} // namespace uos_ai

#endif // COZEAGENTCHAT_H
