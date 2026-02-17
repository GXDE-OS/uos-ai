#include "deepinmultimedia.h"
#include "osinfo.h"

#include <QDBusPendingCall>
#include <QDBusReply>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QProcess>
#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(logOsControl)

using namespace uos_ai;

// DBus 服务名称和路径定义
static const QString MUSIC_SERVICE = "org.mpris.MediaPlayer2.DeepinMusic";
static const QString MUSIC_PATH = "/org/mpris/MediaPlayer2";
static const QString MUSIC_INTERFACE = "org.mpris.MediaPlayer2.Player";

DeepinMultimedia::DeepinMultimedia(QObject *parent) : QObject(parent)
{
    // 初始化音乐播放器 DBus 接口
    m_musicProxy.reset(new QDBusInterface(
        MUSIC_SERVICE,
        MUSIC_PATH,
        MUSIC_INTERFACE,
        QDBusConnection::sessionBus(), this));
}

DeepinMultimedia::~DeepinMultimedia()
{
}

bool DeepinMultimedia::callMusicDBusMethod(QDBusInterface *interface,
                                              const QString &method,
                                              const QVariantList &args,
                                              QString &errorInfo)
{
    if (!interface || !interface->isValid()) {
        m_musicProxy.reset(new QDBusInterface(
            MUSIC_SERVICE,
            MUSIC_PATH,
            MUSIC_INTERFACE,
            QDBusConnection::sessionBus(), this));

        interface = m_musicProxy.data();
    }

    if (!interface || !interface->isValid()) {
        errorInfo = "The D-Bus interface is not valid because the music application hasn't been launched.";
        qCWarning(logOsControl) << "DBus interface is not valid for method:" << method;
        return false;
    }

    QDBusReply<void> reply = interface->callWithArgumentList(QDBus::Block, method, args);
    if (!reply.isValid()) {
        errorInfo = reply.error().message();
        return false;
    }

    return true;
}

bool DeepinMultimedia::stateControl(const QString &control, QString &errorInfo)
{
    qCDebug(logOsControl) << "Controlling playback state:" << control;

    if (control == "Play") {
        return callMusicDBusMethod(m_musicProxy.data(), "Play", QVariantList(), errorInfo);
    } else if (control == "Pause") {
        return callMusicDBusMethod(m_musicProxy.data(), "Pause", QVariantList(), errorInfo);
    } else if (control == "Previous") {
        return callMusicDBusMethod(m_musicProxy.data(), "Previous", QVariantList(), errorInfo);
    } else if (control == "Next") {
        return callMusicDBusMethod(m_musicProxy.data(), "Next", QVariantList(), errorInfo);
    }

    errorInfo = "Invalid control command";
    return false;
}

bool DeepinMultimedia::seek(int offset, QString &errorInfo)
{
    return callMusicDBusMethod(m_musicProxy.data(), "Seek", QVariantList() << static_cast<qlonglong>(offset * 1000), errorInfo);
}
