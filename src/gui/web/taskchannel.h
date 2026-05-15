#ifndef TASKCHANNEL_H
#define TASKCHANNEL_H

#include <QObject>
#include <QPair>
#include <QList>
#include <QMap>
#include <QMutex>

namespace uos_ai {

class WebContext;
class FileChannel;
class WindowChannel;
class MgmtWindow;
class ConversationChannel;

/**
 * @brief Web ↔ Native channel for task operations.
 *
 * Registered as `window.taskObj` in QWebChannel.
 *
 * C++ → JS (callable from C++):
 *   appendImage(imagePath)       Append screenshot image to chat
 *   appendPrompt(question, isSend)  Append prompt to chat
 *   overrideQuestion(question, ext)  Override current chat question with optional file
 *
 * These methods handle queueing when window is not ready and process
 * pending tasks when window is created.
 */
class TaskChannel : public QObject
{
    Q_OBJECT
public:
    explicit TaskChannel(QObject *parent = nullptr);
    
    void setWebContext(WebContext *context);
    void setMgmtWindow(MgmtWindow *mgmtWindow);
    /**
     * @brief Append an image file to the chat
     * @param imagePath Path to the image file to append
     */
    void appendImage(const QString &imagePath);
    
    /**
     * @brief Append a prompt/question to the chat
     * @param question The question text
     * @param isSend Whether to automatically send the question
     */
    void appendPrompt(const QString &question, bool isSend);
    
    /**
     * @brief Override the current chat question
     * @param question The new question text
     * @param ext Optional extensions map (e.g., "file" for file path, "defaultPrompt" for default prompt)
     */
    void overrideQuestion(const QString &question, const QMap<QString, QString> &ext);
    void addKnowledgeBase(const QStringList &knowledgeBaseFiles);
    void changeToConversation(const QString &assistantId, const QString &conversationId);
    void changeToDigitalMode();

public slots:
    void onWindowCreated();
    void processPendingOverrideQuestions();
    void processPendingAppendPrompts();
    void processPendingAppendImages();
    void processPendingAddKnowledgeBases();
    void processPendingChangeToConversations();
    void processPendingChangeToDigitalMode();

signals:
    void taskAdded(int mode);
private:
    FileChannel *m_fileCh = nullptr;
    WindowChannel *m_winCh = nullptr;
    MgmtWindow *m_mgmtWindow = nullptr;
    ConversationChannel *m_conversationCh = nullptr;
    
    
    QList<QString> m_pendingAppendImage;
    QList<QPair<QString, QMap<QString, QString>>> m_pendingOverrideQuestions;
    QList<QPair<QString, bool>> m_pendingAppendPrompt;
    QString m_pendingAssistantId;
    QString m_pendingConversationId;
    bool m_pendingDigitalMode = false;
    
    QMutex m_mutex; // 保护队列的互斥锁
    QList<QStringList> m_pendingAddKnowledgeBases;
};

} // namespace uos_ai

#endif // TASKCHANNEL_H
