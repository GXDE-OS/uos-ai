#ifndef CONVERSATIONMIGRATION_H
#define CONVERSATIONMIGRATION_H

#include "migration.h"
#include "global_define.h"

#include <QList>
#include <QVariantHash>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

namespace uos_ai {
// Local definitions for old format parsing (previously from wrapper/global.h)
// These are only used for migrating old conversation data
// Old ChatChunk struct for parsing old format JSON
struct OldChatChunk
{
    QString displayContent;
    QString content;
    QString displayHash;
    QString reqId;
    QString llmIcon;
    QString llmName;
    QString llmId;
    QString assistantId;
    QString assistantName;
    int llmModel;
    int errCode;
    QString errInfo;
    int chatType;
    QString extention;
    QString thinkTime;
    bool isRetry;
    bool openThink;
    bool onlineSearch;
    bool knowledgeSearchStatus;

    static void json2ChatChunk(const QByteArray &chatChunkData, OldChatChunk &chatChunk) {
        if (chatChunkData.isEmpty())
            return;

        QJsonObject chatChunkObj = QJsonDocument::fromJson(chatChunkData).object();
        chatChunk.displayContent = chatChunkObj.value("displayContent").toString();
        chatChunk.displayHash = chatChunkObj.value("displayHash").toString();
        chatChunk.content = chatChunkObj.value("content").toString();
        chatChunk.reqId = chatChunkObj.value("reqId").toString();
        chatChunk.llmIcon = chatChunkObj.value("llmIcon").toString();
        chatChunk.llmName = chatChunkObj.value("llmName").toString();
        chatChunk.llmId = chatChunkObj.value("llmId").toString();
        chatChunk.llmModel = chatChunkObj.value("llmModel").toInt();
        chatChunk.assistantId = chatChunkObj.value("assistantId").toString();
        chatChunk.assistantName = chatChunkObj.value("assistantName").toString();
        chatChunk.errCode = chatChunkObj.value("errCode").toInt();
        chatChunk.errInfo = chatChunkObj.value("errInfo").toString();
        chatChunk.chatType = chatChunkObj.value("chatType").toInt();
        chatChunk.extention = chatChunkObj.value("extention").toString();
        chatChunk.thinkTime = chatChunkObj.value("thinkTime").toString();
        chatChunk.isRetry = chatChunkObj.value("isRetry").toBool();
        chatChunk.openThink = chatChunkObj.value("openThink").toBool(true);
        chatChunk.onlineSearch = chatChunkObj.value("onlineSearch").toBool(false);
        chatChunk.knowledgeSearchStatus = chatChunkObj.value("knowledgeSearchStatus").toBool(false);
    }
};

// Old Conversations struct for parsing old format JSON
struct OldConversations
{
    OldChatChunk question;
    QVector<OldChatChunk> answers;

    static void json2Convs(QVector<OldConversations> &convs, const QByteArray &convsData)
    {
        if (convsData.isEmpty())
            return;

        QJsonArray convsArray = QJsonDocument::fromJson(convsData).array();
        for (auto conv : convsArray) {
            QJsonObject chatChunkQObj = conv.toObject().value("question").toObject();
            QJsonArray chatChunkAArray = conv.toObject().value("answers").toArray();

            OldChatChunk chatchunkQ;
            OldChatChunk::json2ChatChunk(QJsonDocument(chatChunkQObj).toJson(), chatchunkQ);

            QVector<OldChatChunk> answers;
            for (auto answerObj : chatChunkAArray) {
                OldChatChunk chatchunkA;
                OldChatChunk::json2ChatChunk(QJsonDocument(answerObj.toObject()).toJson(), chatchunkA);
                answers.append(chatchunkA);
            }

            OldConversations conversations;
            conversations.question = chatchunkQ;
            conversations.answers = answers;

            convs.append(conversations);
        }
    }
};

class RenderMessage;
class MetaMessage;
class ConversationMigration : public Migration
{
public:
    QString name() const override;
    bool migrate() override;
    bool isNeeded() const override;

private:
    bool migrateConversationData();

    // Helper methods for parsing old format
    ContentType chatActionToContentType(int chatType);
    QList<RenderMessage> parseDisplayContent(const QString &displayContent, int thinkTime = 0);
    QList<MetaMessage> parseModelContent(const QString &displayContent);
    bool importFromOldFormat(const QString &fileName);
    void importAllFromOldFormat();
};

}

#endif // CONVERSATIONMIGRATION_H
