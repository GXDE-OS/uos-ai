#ifndef FCITXINPUTSERVER_H
#define FCITXINPUTSERVER_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

class FcitxInputServer: public QDBusAbstractInterface
{
    Q_OBJECT
public:
#ifdef COMPILE_ON_V23
    static inline const char *staticServiceName()
    { return "org.fcitx.Fcitx5";}
    static inline const char *staticObjectPath()
    { return "/uosai";}
    static inline const char *staticInterfaceName()
    { return "org.fcitx.Fcitx5.uosai"; }
#else
    static inline const char *staticServiceName()
    { return "org.fcitx.Fcitx";}
    static inline const char *staticObjectPath()
    { return "/uosai";}
    static inline const char *staticInterfaceName()
    { return "org.fcitx.Fcitx.uosai"; }
#endif

    static FcitxInputServer &getInstance();

    ~FcitxInputServer();

private:
    FcitxInputServer(QObject *parent = nullptr);

public Q_SLOTS: // METHODS
    void onFocusIn()
    {
        emit signalFocusIn();
    }

    void onFocusOut()
    {
        emit signalFocusOut();
    }

    inline QDBusPendingReply<bool> commitString(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("PaddingString"), argumentList);
    }

    inline QDBusPendingReply<bool> deleteChar()
    {
        return asyncCall(QStringLiteral("DeleteChar"));
    }

#ifdef COMPILE_ON_V23
    inline QDBusPendingReply<void> setPreEditOn(bool isOn)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(isOn);
        return asyncCallWithArgumentList(QStringLiteral("SetPreEditOn"), argumentList);
    }

    inline QDBusPendingReply<void> commitToPreEdit(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("CommitToPreEdit"), argumentList);
    }
#endif

Q_SIGNALS: // SIGNALS
    void signalFocusIn();
    void signalFocusOut();
};

#endif // FCITXINPUTSERVER_H


