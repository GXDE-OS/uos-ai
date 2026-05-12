#include "taskchannel.h"
#include "webcontext.h"
#include "filechannel.h"
#include "windowchannel.h"
#include "conversationchannel.h"
#include "appwebview.h"
#include "global_define.h"
#include "mgmtwindow.h"

#include <QLoggingCategory>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

TaskChannel::TaskChannel(QObject *parent)
    : QObject(parent)
{
}

void TaskChannel::setMgmtWindow(MgmtWindow *mgmtWindow)
{
    m_mgmtWindow = mgmtWindow;
}

void TaskChannel::setWebContext(WebContext *context)
{
    QMutexLocker locker(&m_mutex);
    if (context) {
        m_fileCh = context->fileCh;
        m_winCh = context->winCh;
        m_conversationCh = context->conversationCh;
    }
}

void TaskChannel::onWindowCreated()
{
    processPendingChangeToDigitalMode();
    processPendingOverrideQuestions();
    processPendingAppendPrompts();
    processPendingAppendImages();
    processPendingAddKnowledgeBases();
    processPendingChangeToConversations();
}

void TaskChannel::appendImage(const QString &imagePath)
{
    QMutexLocker locker(&m_mutex);
    
    qCInfo(logAIGUI) << "TaskChannel: queuing appendImage task";
    m_pendingAppendImage.append(imagePath);
    emit taskAdded(TaskMode::UploadImage);

}

void TaskChannel::appendPrompt(const QString &question, bool isSend)
{
    QMutexLocker locker(&m_mutex);
    
    qCInfo(logAIGUI) << "TaskChannel: queuing appendPrompt task";
    m_pendingAppendPrompt.append(qMakePair(question, isSend));
    emit taskAdded(TaskMode::AddAskQuestion);
}

void TaskChannel::overrideQuestion(const QString &question, const QMap<QString, QString> &ext)
{
    QMutexLocker locker(&m_mutex);
    
    qCInfo(logAIGUI) << "TaskChannel: queuing overrideQuestion task";
    m_pendingOverrideQuestions.append(qMakePair(question, ext));
    emit taskAdded(TaskMode::OverrideQuestion);
}

void TaskChannel::addKnowledgeBase(const QStringList &knowledgeBaseFiles)
{
    QMutexLocker locker(&m_mutex);
    
    qCInfo(logAIGUI) << "TaskChannel: queuing addKnowledgeBase task:" << knowledgeBaseFiles;
    m_pendingAddKnowledgeBases.append(knowledgeBaseFiles);
    emit taskAdded(TaskMode::AddKnowledgeBase);
}

void TaskChannel::changeToConversation(const QString &assistantId, const QString &conversationId)
{
    QMutexLocker locker(&m_mutex);
    
    qCInfo(logAIGUI) << "TaskChannel: queuing changeToConversation task, assistantId:" << assistantId
                     << "conversationId:" << conversationId;
    
    // 只保存最后一个目标ID（因为最终只能跳转到最后一个Conversation）
    m_pendingAssistantId = assistantId;
    m_pendingConversationId = conversationId;
    emit taskAdded(TaskMode::ChangeToConversation);
}

void TaskChannel::changeToDigitalMode()
{
    QMutexLocker locker(&m_mutex);

    qCInfo(logAIGUI) << "TaskChannel: queuing changeToDigitalMode";

    m_pendingDigitalMode = true;
    emit taskAdded(TaskMode::ChangeToDigitalMode);
}


void TaskChannel::processPendingOverrideQuestions()
{
    QList<QPair<QString, QMap<QString, QString>>> tasksToProcess;
    
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pendingOverrideQuestions.isEmpty()) {
            tasksToProcess = m_pendingOverrideQuestions;
            m_pendingOverrideQuestions.clear();
            qCInfo(logAIGUI) << "TaskChannel: Processing" << tasksToProcess.size() << "pending overrideQuestion tasks";
        }
    }
    
    // 处理任务时不需要持有锁，避免死锁
    for (const auto &task : tasksToProcess) {
        QString question = task.first;
        QMap<QString, QString> ext = task.second;

        // Check if we need to handle file parameter
        if (ext.contains("file")) {
            QString filePath = ext["file"];
            QString defaultPrompt = ext["defaultPrompt"];
            qCInfo(logAIGUI) << "TaskChannel: Processing pending overrideQuestion with file. question:" << question
                            << "defaultPrompt:" << defaultPrompt << "file:" << filePath;

            // 只把路径转交给前端，是否接受上传由前端统一判断。
            if (m_fileCh) {
                m_fileCh->emitIncomingFiles(
                    QStringList{ filePath },
                    defaultPrompt,
                    -1,
                    QStringLiteral("handleDroppedFiles")
                );
            }
        }

        emit m_winCh->windowOverrideQuestion(question);
    }
    
    if (!tasksToProcess.isEmpty()) {
        qCInfo(logAIGUI) << "TaskChannel: All pending overrideQuestion tasks processed";
    }
}

void TaskChannel::processPendingAppendPrompts()
{
    QList<QPair<QString, bool>> tasksToProcess;
    
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pendingAppendPrompt.isEmpty()) {
            tasksToProcess = m_pendingAppendPrompt;
            m_pendingAppendPrompt.clear();
            qCInfo(logAIGUI) << "TaskChannel: Processing" << tasksToProcess.size() << "pending appendPrompt tasks";
        }
    }
    
    // 处理任务时不需要持有锁，避免死锁
    for (const auto &task : tasksToProcess) {
        QString question = task.first;
        bool isSend = task.second;
        emit m_winCh->windowAppendPrompt(question, isSend);
    }
    
    if (!tasksToProcess.isEmpty()) {
        qCInfo(logAIGUI) << "TaskChannel: All pending appendPrompt tasks processed";
    }
}

void TaskChannel::processPendingAppendImages()
{
    QList<QString> tasksToProcess;
    
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pendingAppendImage.isEmpty()) {
            tasksToProcess = m_pendingAppendImage;
            m_pendingAppendImage.clear();
            qCInfo(logAIGUI) << "TaskChannel: Processing" << tasksToProcess.size() << "pending appendImage tasks";
        }
    }
    
    // 处理任务时不需要持有锁，避免死锁
    for (const auto &task : tasksToProcess) {
        QString imagePath = task;
        if (m_fileCh) {
            m_fileCh->emitIncomingFiles(
                QStringList{ imagePath },
                QString(),
                -1,
                QStringLiteral("handleScreenshotFile")
            );
        }
    }
    
    if (!tasksToProcess.isEmpty()) {
        qCInfo(logAIGUI) << "TaskChannel: All pending appendImage tasks processed";
    }
}

void TaskChannel::processPendingAddKnowledgeBases()
{
    QList<QStringList> tasksToProcess;
    
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pendingAddKnowledgeBases.isEmpty()) {
            tasksToProcess = m_pendingAddKnowledgeBases;
            m_pendingAddKnowledgeBases.clear();
            qCInfo(logAIGUI) << "TaskChannel: Processing" << tasksToProcess.size() << "pending addKnowledgeBase tasks";
        }
    }
    
    // 处理任务时不需要持有锁，避免死锁
    if (m_mgmtWindow) {
        for (const auto &task : tasksToProcess) {
            qCInfo(logAIGUI) << "TaskChannel: Processing pending addKnowledgeBase with file:" << task;
            m_mgmtWindow->showPage(MgmtWindow::KnowledgeBase);
            m_mgmtWindow->onAddKnowledgeBase(task);
        }
    } else {
        qCWarning(logAIGUI) << "TaskChannel: Cannot process addKnowledgeBase - m_mgmtWindow is null";
    }
    
    if (!tasksToProcess.isEmpty()) {
        qCInfo(logAIGUI) << "TaskChannel: All pending addKnowledgeBase tasks processed";
    }
}

void TaskChannel::processPendingChangeToConversations()
{
    QString assistantId;
    QString conversationId;
    
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pendingConversationId.isEmpty()) {
            assistantId = m_pendingAssistantId;
            conversationId = m_pendingConversationId;
            m_pendingAssistantId.clear();
            m_pendingConversationId.clear();
            qCInfo(logAIGUI) << "TaskChannel: Processing pending changeToConversation, assistantId:"
                            << assistantId << "conversationId:" << conversationId;
        }
    }
    
    // 处理任务时不需要持有锁，避免死锁
    if (!conversationId.isEmpty() && m_conversationCh) {
        emit m_conversationCh->changeToConversation(assistantId, conversationId);
        qCInfo(logAIGUI) << "TaskChannel: changeToConversation task processed";
    } else if (!conversationId.isEmpty()) {
        qCWarning(logAIGUI) << "TaskChannel: Cannot process changeToConversation - m_conversationCh is null";
    }
}

void TaskChannel::processPendingChangeToDigitalMode()
{
    bool isDigitalMode = false;
    {
        QMutexLocker locker(&m_mutex);
        if (m_pendingDigitalMode) {
            isDigitalMode = m_pendingDigitalMode;
            m_pendingDigitalMode = false;
            qCInfo(logAIGUI) << "TaskChannel: Processing pending changeToDigitalMode";
        }
    }

    if (isDigitalMode) {
        emit m_winCh->windowChangeToDigitalMode();
        qCInfo(logAIGUI) << "TaskChannel: changeToDigitalMode task processed";
    }
}
