#include "networkmonitor.h"

#include <QDebug>
#include <QtDBus>
#include <QMutexLocker>

#define NM_SERVICE      "org.freedesktop.NetworkManager"
#define NM_PATH         "/org/freedesktop/NetworkManager"
#define NM_INTERFACE    "org.freedesktop.NetworkManager"

enum NMState {
    NM_STATE_UNKNOWN = 0,
    NM_STATE_ASLEEP = 10,
    NM_STATE_DISCONNECTED = 20,
    NM_STATE_DISCONNECTING = 30,
    NM_STATE_CONNECTING = 40,
    NM_STATE_CONNECTED_LOCAL = 50,
    NM_STATE_CONNECTED_SITE = 60,
    NM_STATE_CONNECTED_GLOBAL = 70
};

NetworkMonitor::NetworkMonitor()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE,
                                                      NM_PATH,
                                                      NM_INTERFACE,
                                                      "state");

    QDBusReply<quint32> ret = QDBusConnection::systemBus().call(msg);
    if (!ret.isValid()) {
        qCritical() << "dbus call network manager state failed";
    } else {
        m_online = (ret.value() == NM_STATE_CONNECTED_GLOBAL);
        m_state = ret.value();
        qDebug() << "network status:" << (m_online ? "online" : "offline");
    }

    QDBusConnection::systemBus().connect(NM_SERVICE,
                                         NM_PATH,
                                         NM_INTERFACE,
                                         "StateChanged",
                                         this, SLOT(onNMStateChanged(quint32)));

}

NetworkMonitor::~NetworkMonitor()
{
    QDBusConnection::systemBus().disconnect(NM_SERVICE,
                                            NM_PATH,
                                            NM_INTERFACE,
                                            "StateChanged",
                                            this, SLOT(onNMStateChanged(quint32)));
}

NetworkMonitor &NetworkMonitor::getInstance()
{
    static NetworkMonitor instance;
    return instance;
}

bool NetworkMonitor::isOnline()
{
    return m_online;
}

void NetworkMonitor::onNMStateChanged(quint32 state)
{
    bool online = false;

    if (state == NM_STATE_CONNECTED_GLOBAL) {
        online = true;
    }

    if (online != m_online) {
        m_online = online;
        emit stateChanged(online);
    }
}

quint32 NetworkMonitor::checkNetworkState()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE,
                                                      NM_PATH,
                                                      NM_INTERFACE,
                                                      "state");

    QDBusReply<quint32> ret = QDBusConnection::systemBus().call(msg);
    quint32 retValue = 0;
    if (!ret.isValid()) {
        qCritical() << "dbus call network manager state failed";
    } else {
        retValue = ret.value();
    }
    return retValue;  //70连接成功 40正在连接  其他网络断开连接
}
