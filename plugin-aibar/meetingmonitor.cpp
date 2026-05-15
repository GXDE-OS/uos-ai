#include "meetingmonitor.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

using namespace uos_ai;
//******************************************************************************
MeetingMonitor::MeetingMonitor()
{
    timer.setInterval(5000);
    connect(&timer, &QTimer::timeout, this, &MeetingMonitor::onReadProcDir);
    // timer.start();
}

void MeetingMonitor::onStartAiMeeting()
{
    QProcess prrcess;
    prrcess.setProgram("sh"); // 设置程序为sh
    prrcess.setArguments({"-c", "nohup /opt/apps/com.aimeetingassistant.uos/files/run.sh > /dev/null 2>&1 &"}); // 设置参数为-c和包含管道的命令
    prrcess.setProcessChannelMode(QProcess::MergedChannels);
    prrcess.start();
    
    if (!prrcess.waitForFinished()) {
        qCWarning(logAIBar) << "Failed to start meeting assistant process";
    } else {
        qCDebug(logAIBar) << "Meeting assistant process started successfully";
    }
}

void MeetingMonitor::onReadProcDir()
{
    bool meetingAssistantRunning = false;
    QDir procDir("/proc");
    // 遍历/proc目录下的所有子目录
    QFileInfoList procList = procDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QFileInfo procInfo, procList) {
        // 构建cmdline文件的路径
        QString cmdlinePath = procInfo.filePath() + "/cmdline";
        QFile cmdlineFile(cmdlinePath);
        if (cmdlineFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&cmdlineFile);
            // 读取cmdline文件内容
            QString cmdline = in.readAll();
            if(cmdline.contains(meetingName)){
                qCDebug(logAIBar) << "Found meeting assistant process";
                meetingAssistantRunning = true;
                cmdlineFile.close();
                break;
            }
            cmdlineFile.close();
        }
    }
    if(meetingAssistantRunning){
        if(!m_meetingAssistantRunning){
            qCDebug(logAIBar) << "Meeting assistant started running";
            emit sigIsMileMeetingRunning(true);
            m_meetingAssistantRunning = true;
        }
    }else {
        if(m_meetingAssistantRunning){
            qCDebug(logAIBar) << "Meeting assistant stopped running";
            emit sigIsMileMeetingRunning(false);
            m_meetingAssistantRunning = false;
        }
    }
}
