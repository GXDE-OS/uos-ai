#include "audiointerface.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusReply>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

using namespace uos_ai;
AudioInterface::AudioInterface()
{
    m_meetingApps << "会议" << "Wemeet VoiceEngine";
    timer.setInterval(5000);
    connect(&timer, &QTimer::timeout, this, &AudioInterface::getAllSinkName);
    qCDebug(logAIBar) << "Monitoring meeting apps:" << m_meetingApps;
}

void AudioInterface::propertiesChanged(QString interface, QVariantMap changedProperties, QStringList)
{
    if (interface != "org.deepin.dde.Audio1"){
        qCDebug(logAIBar) << "Ignoring properties change for interface:" << interface;
        return;
    }

    for (auto iter = changedProperties.begin(); iter != changedProperties.end(); iter++) {
        if (iter.key() == "SinkInputs") {
            qCDebug(logAIBar) << "Detected sink inputs change";
            auto objs = qdbus_cast<QList<QDBusObjectPath>>(iter.value());
            QStringList path;
            for (auto obj : objs){
                path << obj.path();
            }
            detectedMeetingSceneFirst(path);
        }
    }
}

void AudioInterface::getAllSinkName()
{
    // 连接到DBus系统总线，这里使用session bus
    QDBusConnection connection = QDBusConnection::sessionBus();

    // 创建一个DBus接口
   // 请确保com.deepin.daemon.Audio和org.freedesktop.DBus.Properties是你的服务名和接口名
    QDBusInterface interface("org.deepin.dde.Audio1",
                             "/org/deepin/dde/Audio1",
                             "org.freedesktop.DBus.Properties",
                             connection);
    QDBusMessage message = QDBusMessage::createMethodCall(
                "org.deepin.dde.Audio1",    // 服务名
                "/org/deepin/dde/Audio1",   // 对象路径
                "org.freedesktop.DBus.Properties", // 接口名
                "Get"                      // 方法名
                );
    QVariantList params;
    params << QVariant::fromValue(QString("org.deepin.dde.Audio1"));
    params << QVariant::fromValue(QString("SinkInputs"));
    message.setArguments(params);

    QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(message);
    auto objs = qdbus_cast< QList<QDBusObjectPath> >(reply.value());
    QStringList paths;
    for (auto obj : objs){
        paths << obj.path();
    }

    qCDebug(logAIBar) << "Found" << paths.size() << "audio sinks";
    detectedMeetingScene(paths);
}

QString AudioInterface::getSinkName(const QString &path)
{
    auto msg = QDBusMessage::createMethodCall("org.deepin.dde.Audio1", path, "org.freedesktop.DBus.Properties"
                                              , "Get");
    QVariantList params;
    params << QVariant::fromValue(QString("org.deepin.dde.Audio1.SinkInput"));
    params << QVariant::fromValue(QString("Name"));
    msg.setArguments(params);

    QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(msg);

    return reply.value().toString();
}

bool AudioInterface::detectedAiMeetingIsInstalled()
{
    QString filePath = "/opt/apps/com.aimeetingassistant.uos/files/run.sh";
    QFile file(filePath);
    bool exists = file.exists();
    qCDebug(logAIBar) << "Meeting assistant installation check:" << exists;
    return exists;
}

void AudioInterface::detectedMeetingScene(const QStringList &paths)
{
    if(!detectedAiMeetingIsInstalled()){
        qCDebug(logAIBar) << "Meeting assistant not installed, skipping detection";
        return;
    }

    bool isDetectedMeetingScene = false;
    for (auto path : paths){
        QString sinkName = getSinkName(path);
        if(m_meetingApps.contains(sinkName)){
            qCDebug(logAIBar) << "Detected meeting app:" << sinkName;
            isDetectedMeetingScene = true;
            break;
        }
    }

    if(isDetectedMeetingScene){
        if(!m_isDetectedMeetingScene){
            qCInfo(logAIBar) << "Entering meeting scene";
            m_isDetectedMeetingScene = true;
            // QMetaObject::invokeMethod(&timer, "start", Qt::QueuedConnection);
            //当前不在会议场景，提交信号到主线程，通知前端显示
            emit sigMeetingSceneChanged(true);
        }
    } else {
        if(m_isDetectedMeetingScene){
            qCInfo(logAIBar) << "Exiting meeting scene";
            m_isDetectedMeetingScene = false;
            //当前在会议场景，提交信号到主线程，通知前端判断是否隐藏
            emit sigMeetingSceneChanged(false);
        }
        if (timer.isActive()){
            qCDebug(logAIBar) << "Stopping detection timer";
            QMetaObject::invokeMethod(&timer, "stop", Qt::QueuedConnection);
        }
    }
}

void AudioInterface::detectedMeetingSceneFirst(const QStringList &paths)
{
    bool isDetectedMeetingScene = false;
    for ( auto path : paths){  //需要遍历完才能知道是否存在会议场景
        QString sinkName = getSinkName(path);
        if(m_meetingApps.contains(sinkName)){  //发现会议场景
            isDetectedMeetingScene = true;
            break;
        }
    }
    if(isDetectedMeetingScene){  //上面找到会议场景
        if(!m_isDetectedMeetingScene){
            if (!timer.isActive())
            {
                QMetaObject::invokeMethod(&timer, "start", Qt::QueuedConnection);
            }
        }
    }
}