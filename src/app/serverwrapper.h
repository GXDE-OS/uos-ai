#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include "audioaiassistant.h"

#include <QSharedPointer>
#include <QJsonArray>

namespace uos_ai {
class DBusInterface;
class ChatDBusInterface;

class ServerWrapper : public QObject
{
    Q_OBJECT
public:
    ServerWrapper();
    static ServerWrapper *instance();

public:
    /**
     * @brief registerService
     * @return
     */
    static bool registerService();

    /**
     * @brief initialization Register for dubs services and objects
     */
    bool initialization();

    void updateVisibleState(bool visible);
    void updateActiveState(bool active);
private:
    QSharedPointer<uos_ai::DBusInterface> m_copilotDbusObject;
    QSharedPointer<ChatDBusInterface> m_chatDbusObject;
    AudioAiassistant *m_audioAiassistant = nullptr;
};

}
#endif // SERVERWRAPPER_H
