#ifndef CONVERSATIONCHANNEL_H
#define CONVERSATIONCHANNEL_H

#include <QObject>
#include <QJsonObject>
#include <QLoggingCategory>

namespace uos_ai {

class ConversationChannel : public QObject
{
    Q_OBJECT
public:
    explicit ConversationChannel(QObject *parent = nullptr);
    ~ConversationChannel() override;

public slots:
    // ── 对话管理 ────────────────────────────────────────────────────────────
    QJsonObject getConversation(const QString &id);
    void deleteConversation(const QStringList &ids);
    void releaseConversation(const QStringList &ids);
    void clearAllConversations();
    void setConversationRender(const QString &conversationId, const QString &msgId, const QString &renderMsgJson);
    bool saveConversation(const QString &id);
    QString getConversationIndexes();
    bool switchMessageNext(const QString &conversationId, const QString &target, const QString &next);

    // ── AI 写作工作区 ────────────────────────────────────────────────────────
    void updateWorkspaceOutline(const QString &conversationId, const QString &outlineJson);
    QString getWorkspaceOutline(const QString &conversationId, const QString &articleId);
    QString getWorkspaceArticle(const QString &conversationId, const QString &articleId, int version = -1);
    bool updateWorkspaceArticle(const QString &conversationId, const QString &articleId, const QString &content);
    bool saveWorkspaceArticleToFile(const QString &conversationId, const QString &articleId, const QString &format);

    // ── 文档打印 ────────────────────────────────────────────────────────
    void printHTML(const QString &html, const QString &title);

signals:
    void indexChanged();
    void changeToConversation(const QString &assistantId, const QString &conversationId);

private:
    bool m_isPrinting = false;
};

} // namespace uos_ai

#endif // CONVERSATIONCHANNEL_H
