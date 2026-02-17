#include "bus.h"
#include "eim.h"
#include <fcitx/module/dbus/fcitx-dbus.h>
#include <fcitx-utils/log.h>
// must keep X11 haeder under QT header
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

const char *introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node name=\"" FCITX_UOSAI_PATH "\">\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" FCITX_UOSAI_INTERFACE "\">\n"
    "    <method name=\"PaddingString\">"
    "      <arg name=\"str\" direction=\"in\" type=\"s\"/>\n"
    "    </method>"
    "    <method name=\"DeleteChar\">"
    "    </method>"
    "    <signal name=\"SignalFocusIn\"/>"
    "    <signal name=\"SignalFocusOut\"/>"
    "  </interface>\n"
    "</node>\n";

static DBusHandlerResult dbusEventHandler(DBusConnection* conn, DBusMessage* message, void* self) {
    FcitxUosAiBus* bus = (FcitxUosAiBus*) self;
    return bus->dbusEvent(conn, message);
}

void onInputFocusIn(void *arg) {
    FcitxUosAiBus *bus = static_cast<FcitxUosAiBus*>(arg);
    if (bus) {
        bus->sendFocusInSignal();
    }
}

void onInputFocusOut(void *arg) {
    FcitxUosAiBus *bus = static_cast<FcitxUosAiBus*>(arg);
    if (bus) {
        bus->sendFocusOutSignal();
    }
}

FcitxUosAiBus::FcitxUosAiBus(struct _FcitxUosAiAddonInstance* uosai)
{
    FcitxLog(INFO, "UosAi: Initializing UosAi bus");
    FcitxInstance* instance = uosai->owner;
    DBusConnection *conn = FcitxDBusGetConnection(instance);
    DBusConnection *privconn = FcitxDBusGetPrivConnection(instance);
    if (conn == NULL && privconn == NULL) {
        FcitxLog(ERROR, "UosAi: DBus Not initialized");
    }

    m_uosai = uosai;
    m_conn = conn;
    m_privconn = privconn;

    DBusObjectPathVTable fcitxIPCVTable = {NULL, &dbusEventHandler, NULL, NULL, NULL, NULL };

    if (conn) {
        dbus_connection_register_object_path(conn, FCITX_UOSAI_PATH, &fcitxIPCVTable, this);
    }

    if (privconn) {
        dbus_connection_register_object_path(privconn, FCITX_UOSAI_PATH, &fcitxIPCVTable, this);
    }

    FcitxIMEventHook focusInHook = {onInputFocusIn, this};
    FcitxIMEventHook focusOutHook = {onInputFocusOut, this};

    FcitxInstanceRegisterInputFocusHook(m_uosai->owner, focusInHook);
    FcitxInstanceRegisterInputUnFocusHook(m_uosai->owner, focusOutHook);

    FcitxLog(INFO, "UosAi: UosAi bus initialization completed");
    printf("FcitxUosAiBus(struct _FcitxUosAiAddonInstance* uosai)\n");
}

FcitxUosAiBus::~FcitxUosAiBus()
{
    FcitxLog(INFO, "UosAi: Destroying UosAi bus");
    if (m_conn) {
        dbus_connection_unregister_object_path(m_conn, FCITX_UOSAI_PATH);
    }
    if (m_privconn) {
        dbus_connection_unregister_object_path(m_privconn, FCITX_UOSAI_PATH);
    }
}

DBusHandlerResult FcitxUosAiBus::dbusEvent(DBusConnection* connection, DBusMessage* message)
{
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage *reply = NULL;
    boolean flush = false;
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(message);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(message, FCITX_UOSAI_INTERFACE, "DeleteChar")) {
        Display* disp = XOpenDisplay(nullptr);

        if(disp != nullptr) {
            KeyCode key = XKeysymToKeycode(disp, XK_BackSpace);

            XTestFakeKeyEvent(disp, key, true, CurrentTime);
            XTestFakeKeyEvent(disp, key, false, CurrentTime);

            XFlush(disp);

            XCloseDisplay(disp);
        } else {
            FcitxLog(ERROR, "UosAi: Failed to open X display for DeleteChar operation");
        }
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, FCITX_UOSAI_INTERFACE, "PaddingString")) {
        char* str = NULL;
        if (dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID)) {
            FcitxInstanceCommitString(m_uosai->owner, FcitxInstanceGetCurrentIC(m_uosai->owner), str);
            reply = dbus_message_new_method_return(message);
        }
        else {
            FcitxLog(ERROR, "UosAi: Failed to get arguments for PaddingString method");
            reply = dbus_message_new_error_printf(message,
                                                  DBUS_ERROR_UNKNOWN_METHOD,
                                                  "No such method with signature (%s)",
                                                  dbus_message_get_signature(message));
        }
    } 

    if (reply) {
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        if (flush) {
            dbus_connection_flush(connection);
        }
        result = DBUS_HANDLER_RESULT_HANDLED;
    }
    return result;
}

void FcitxUosAiBus::sendFocusInSignal() {
    if (m_conn) {
        DBusMessage *signal = dbus_message_new_signal(
            FCITX_UOSAI_PATH,
            FCITX_UOSAI_INTERFACE,
            "SignalFocusIn"
        );
        dbus_connection_send(m_conn, signal, NULL);
        dbus_message_unref(signal);
    } else {
        FcitxLog(WARNING, "UosAi: Cannot send focus in signal - no DBus connection");
    }
}

void FcitxUosAiBus::sendFocusOutSignal() {
    if (m_conn) {
        DBusMessage *signal = dbus_message_new_signal(
            FCITX_UOSAI_PATH,
            FCITX_UOSAI_INTERFACE,
            "SignalFocusOut"
        );
        dbus_connection_send(m_conn, signal, NULL);
        dbus_message_unref(signal);
    } else {
        FcitxLog(WARNING, "UosAi: Cannot send focus out signal - no DBus connection");
    }
}
