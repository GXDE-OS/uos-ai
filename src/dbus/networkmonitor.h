#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QObject>

class NetworkMonitor : public QObject
{
    Q_OBJECT

public:
    static NetworkMonitor &getInstance();
    virtual ~NetworkMonitor();

    /**
     * @brief checkNetworkState
     * @return
     */
    static quint32 checkNetworkState();

    /**
     * @brief isOnline
     * @return
     */
    bool isOnline();

signals:
    /**
     * @brief stateChanged
     * @param online
     */
    void stateChanged(bool online);

private slots:
    /**
     * @brief onNMStateChanged
     * @param state
     */
    void onNMStateChanged(quint32 state);

private:
    explicit NetworkMonitor();

    bool    m_online{ false };
    quint32 m_state{20};
};

#endif // NETWORKMONITOR_H
