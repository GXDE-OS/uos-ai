#ifndef SESSIONCHANNEL_H
#define SESSIONCHANNEL_H

#include <QObject>
#include <QVariantMap>

namespace uos_ai {

class SessionChannel : public QObject
{
    Q_OBJECT
public:
    explicit SessionChannel(QObject *parent = nullptr);
    ~SessionChannel() override;

signals:
    void sessionEvent(int event, const QString &id, const QString &json);
public slots:
    void sendMessage(const QString &params);
    void retry(const QString &params);
    void cancel(const QString &params);
    void invokeAction(const QString &id, const QString &json);
};

} // namespace uos_ai

#endif // INTERACTION_CHANNEL_SESSIONCHANNEL_H
