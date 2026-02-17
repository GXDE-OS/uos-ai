#ifndef MEETINGASSISTANT_H
#define MEETINGASSISTANT_H

#include <QObject>
#include <QThread>

#include "audiointerface.h"
#include "meetingmonitor.h"

namespace uos_ai {
class MeetingAssistant : public QObject
{
    Q_OBJECT
public:
    enum MeetAssistantStatus{
        HideAll = 1,
        ShowIcon,
        ShowAll
    };
    enum Role{
        MeetingAssistantRole = 1,
        MeetingSceneRole
    };
public:
    MeetingAssistant();
    virtual ~MeetingAssistant() {}
    MeetAssistantStatus getNowMeetAssistantStatus();
    void onClickRecommend();
signals:
    void sigStartAiMeeting();
    void sigMeetAssistantStatusChanged(MeetAssistantStatus status);
private slots:
    void onMeetingSceneChanged(bool isMeetingScene);
    void onIsMileMeetingRunning(bool isRunning);
private:
    void statusChanged(Role role);  //会议助手和会议场景变化
private:
    QThread workThread;
    AudioInterface m_audioInterface;
    MeetingMonitor m_meetingMonitor;

    bool m_meetingAssistantRunning = false;  //会议助手是否启动
    bool m_isDetectedMeetingScene = false;  //是否检测到会议场景
};
}
#endif // MEETINGASSISTANT_H
