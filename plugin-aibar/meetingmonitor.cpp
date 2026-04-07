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
    // 直接执行脚本文件，移除 sh -c 包装，避免命令注入风险
    QString scriptPath = "/opt/apps/com.aimeetingassistant.uos/files/run.sh";
    qint64 pid = -1;

    // 使用 startDetached 直接启动脚本在后台运行，使用 QProcess 处理输出重定向
    bool ok = QProcess::startDetached(
        scriptPath,      // 直接执行脚本
        QStringList(),    // 无参数
        QString(),        // 使用脚本所在目录作为工作目录
        &pid              // 返回进程 ID
    );

    if (!ok || pid < 0) {
        qCWarning(logAIBar) << "Failed to start meeting assistant process";
    } else {
        qCDebug(logAIBar) << "Meeting assistant process started successfully, pid:" << pid;
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
