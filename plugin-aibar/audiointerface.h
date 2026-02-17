#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QVariantMap>
#include <QDBusMessage>
#include <QTimer>
#include <QFile>

namespace uos_ai {
class AudioInterface : public QObject
{
    Q_OBJECT
public:
    AudioInterface();
    virtual ~AudioInterface() {
    }

signals:
    void sigMeetingSceneChanged(bool);
protected slots:
    void propertiesChanged(QString, QVariantMap, QStringList);

private:
    void getAllSinkName();
    QString getSinkName(const QString &path);
    bool detectedAiMeetingIsInstalled();  //判断会议助手是否安装
    void detectedMeetingScene(const QStringList &paths); //检测会议场景变化
    void detectedMeetingSceneFirst(const QStringList &paths); //首次检测会议场景变化

private:
    QTimer timer;
    QStringList m_meetingApps;
    bool m_isDetectedMeetingScene = false;  //是否检测到会议场景
};
}
#endif // AUDIOINTERFACE_H
