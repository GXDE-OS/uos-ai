#include "meetingassistant.h"
#include <QLoggingCategory>
#include <QtDBus>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

using namespace uos_ai;
MeetingAssistant::MeetingAssistant()
{
    if (true)
    {
        return;
    }
    
    connect(&m_audioInterface, &AudioInterface::sigMeetingSceneChanged, this, &MeetingAssistant::onMeetingSceneChanged);
    m_audioInterface.moveToThread(&workThread);

    connect(&m_meetingMonitor, &MeetingMonitor::sigIsMileMeetingRunning, this, &MeetingAssistant::onIsMileMeetingRunning);
    connect(this, &MeetingAssistant::sigStartAiMeeting, &m_meetingMonitor, &MeetingMonitor::onStartAiMeeting);
    m_meetingMonitor.moveToThread(&workThread);

    workThread.start();
}

MeetingAssistant::MeetAssistantStatus MeetingAssistant::getNowMeetAssistantStatus()
{
    if (true)
    {
        return HideAll;
    }
    
    if(m_isDetectedMeetingScene){
        if(m_meetingAssistantRunning){
            return ShowIcon;
        }else {
            return ShowAll;
        }
    }else {
        if(m_meetingAssistantRunning){
            return ShowIcon;
        }else {
            return HideAll;
        }
    }
}

void MeetingAssistant::onMeetingSceneChanged(bool isMeetingScene)
{
    qCDebug(logAIBar) << "Meeting scene changed:" << isMeetingScene;
    m_isDetectedMeetingScene = isMeetingScene;
    statusChanged(MeetingSceneRole);
}

void MeetingAssistant::onIsMileMeetingRunning(bool isRunning)
{
    qCDebug(logAIBar) << "Meeting assistant running state changed:" << isRunning;
    m_meetingAssistantRunning = isRunning;
    statusChanged(MeetingAssistantRole);
}

void MeetingAssistant::onClickRecommend()
{
    qCInfo(logAIBar) << "Recommend button clicked, starting AI meeting";
    emit sigStartAiMeeting();
}

void MeetingAssistant::statusChanged(Role role)
{
    qCDebug(logAIBar) << "Status changed - Role:" << role 
                                << "Meeting scene:" << m_isDetectedMeetingScene
                                << "Assistant running:" << m_meetingAssistantRunning;

    if(m_isDetectedMeetingScene){
        if(m_meetingAssistantRunning){
            //给qml发送信号：AI Bar显示会议助手图标
            qCDebug(logAIBar) << "Emitting ShowIcon status";
            emit sigMeetAssistantStatusChanged(ShowIcon);
        }else {
            //给qml发送信号：AI Bar显示会议助手图标 + 显示推荐交互
            if(role == MeetingSceneRole){
                qCDebug(logAIBar) << "Emitting ShowAll status";
                emit sigMeetAssistantStatusChanged(ShowAll);
            }
        }
    }else {
        if(m_meetingAssistantRunning){
            //给qml发送信号：AI Bar显示会议助手图标
            qCDebug(logAIBar) << "Emitting ShowIcon status";
            emit sigMeetAssistantStatusChanged(ShowIcon);
        }else {
            //给qml发送信号：AI Bar隐藏会议助手图标
            qCDebug(logAIBar) << "Emitting HideAll status";
            emit sigMeetAssistantStatusChanged(HideAll);
        }
    }
}
