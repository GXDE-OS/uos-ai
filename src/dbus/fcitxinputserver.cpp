#include "fcitxinputserver.h"

#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDebug>

FcitxInputServer::FcitxInputServer(QObject *parent)
    : QDBusAbstractInterface(staticServiceName(), staticObjectPath(), staticInterfaceName(), QDBusConnection::sessionBus(), parent)
{
    if (!QDBusConnection::sessionBus().connect(this->service(), this->path(), this->interface(), "SignalFocusIn",
                                               this, SLOT(onFocusIn()))) {
        qWarning() << "the connection was fail!" << "path: " << this->path() << "interface: " << this->interface();
    }
    if (!QDBusConnection::sessionBus().connect(this->service(), this->path(), this->interface(), "SignalFocusOut",
                                               this, SLOT(onFocusOut()))) {
        qWarning() << "the connection was fail!" << "path: " << this->path() << "interface: " << this->interface();
    }
}

FcitxInputServer::~FcitxInputServer()
{
    QDBusConnection::sessionBus().disconnect(service(), path(),
                                             staticInterfaceName(), "SignalFocusIn",
                                             this, SLOT(onFocusIn()));
    QDBusConnection::sessionBus().disconnect(service(), path(),
                                             staticInterfaceName(), "SignalFocusOut",
                                             this, SLOT(onFocusOut()));
}

FcitxInputServer &FcitxInputServer::getInstance()
{
    static FcitxInputServer instance;
    return instance;
}
