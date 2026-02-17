#ifndef FCITX_UOSAI_BUS_H
#define FCITX_UOSAI_BUS_H

#include <dbus/dbus.h>
#include <fcitx/instance.h>
#include <fcitx/hook.h>

class FcitxUosAiBus {
public:
    FcitxUosAiBus(struct _FcitxUosAiAddonInstance* uosai);
    virtual ~FcitxUosAiBus();

    void sendFocusInSignal();
    void sendFocusOutSignal();

    DBusHandlerResult dbusEvent(DBusConnection* connection, DBusMessage* message);
private:
    DBusConnection* m_privconn;
    DBusConnection* m_conn;
    _FcitxUosAiAddonInstance* m_uosai;

};

#endif // FCITX_UOSAI_BUS_H
