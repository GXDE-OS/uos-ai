#ifndef GLOBAL_H
#define GLOBAL_H

#include "serverdefs.h"

#include <QString>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>
#include <QImage>
#include <QPixmap>
#include <QBuffer>

namespace uos_ai {
enum PromptType {
    DefaultChat = 1,
    SystemAssistant,
    PersonalAssistant,
    WordSelectPrompt,
};

enum ExtentionType {
    None,
    Conversation = 1,
    DocSummary,
    WordSelection,
    KnowledgeBase,  // 查看文档来源
    Instruction,
    PictureId,
    LikeOrNot,
    FunctionButton,
    MCPStatus,
    KnowledgeBaseBtnStatus,  // 知识库开关
};
inline constexpr char kExtentionType[] = "type";

enum ExtFileType {
    document = 0,
    image    = 1,
};
inline constexpr char kExtFile[] = "files";
inline constexpr char kExtFileType[] = "type";
inline constexpr char kExtFileContent[] = "content";
inline constexpr char kExtFileMetaInfo[] = "metaInfo";
inline constexpr char kExtFileMetaInfoDocPath[] = "docPath";

struct ChatChunk
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
    QString errInfo; /*JSON*/
    ChatAction chatType; /*问答、文生图、FunctionCall*/
    QString extention;
    QString thinkTime;  //思考时间
    bool isRetry;  /* answer使用 */
    bool openThink;
    bool onlineSearch;
    bool knowledgeSearchStatus;  //当前知识库按钮状态
    /*
        JSONArray
        [
            {
                type: ExtentionType
                docPath: "";
                docName
                iconData
                docContent

                ------------
                Type：
            },
            {
                Type: 标签
                name, icon
            },
            {
                Type: 指令
                name, icon
            },
            {
                Type: PPT
                TODO
            },
            {
                Type: 超链接
                [{name, content}]
            }，
            {
                Type: 预览文档
                [
                    {name, content}
                ]
            },
            {
                Type: 赞、踩
                status
            },
            {
                type: 4,
                sources: [
                            {
                                docPath: "",
                                docContents: [
                                                "...",
                                                "..."
                                             ]
                             },
                             {
                                docPath: "",
                                docContents: [
                                                "...",
                                                "..."
                                             ]
                             }
                          ]
             }
        ]
    */
    static void json2ChatChunk(const QByteArray &chatChunkData, ChatChunk &chatChunk) {
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
        chatChunk.chatType = ChatAction(chatChunkObj.value("chatType").toInt());
        chatChunk.extention = chatChunkObj.value("extention").toString();
        chatChunk.thinkTime = chatChunkObj.value("thinkTime").toString();
        chatChunk.isRetry = chatChunkObj.value("isRetry").toBool();
        chatChunk.openThink = chatChunkObj.value("openThink").toBool(true);
        chatChunk.onlineSearch = chatChunkObj.value("onlineSearch").toBool(false);
        chatChunk.knowledgeSearchStatus = chatChunkObj.value("knowledgeSearchStatus").toBool(false);
    }

    static void chatChunk2Json(QJsonObject &chatChunkObj, const ChatChunk &chatChunk)
    {
        chatChunkObj.insert("displayContent", chatChunk.displayContent);
        chatChunkObj.insert("displayHash", chatChunk.displayHash);
        chatChunkObj.insert("content", chatChunk.content);
        chatChunkObj.insert("reqId", chatChunk.reqId);
        chatChunkObj.insert("llmIcon", chatChunk.llmIcon);
        chatChunkObj.insert("llmName", chatChunk.llmName);
        chatChunkObj.insert("llmId", chatChunk.llmId);
        chatChunkObj.insert("llmModel", chatChunk.llmModel);
        chatChunkObj.insert("assistantId", chatChunk.assistantId);
        chatChunkObj.insert("assistantName", chatChunk.assistantName);
        chatChunkObj.insert("errCode", chatChunk.errCode);
        chatChunkObj.insert("errInfo", chatChunk.errInfo);
        chatChunkObj.insert("chatType", chatChunk.chatType);
        chatChunkObj.insert("extention", chatChunk.extention);
        chatChunkObj.insert("thinkTime", chatChunk.thinkTime);
        chatChunkObj.insert("isRetry", chatChunk.isRetry);
        chatChunkObj.insert("openThink", chatChunk.openThink);
        chatChunkObj.insert("onlineSearch", chatChunk.onlineSearch);
        chatChunkObj.insert("knowledgeSearchStatus", chatChunk.knowledgeSearchStatus);
    }
};

struct Conversations
{
    ChatChunk question;
    QVector<ChatChunk> answers;

    static void convs2Json(QByteArray &convsData, const QVector<Conversations> &convs)
    {
        QJsonArray convsArray;
        for (auto conv : convs) {
            QJsonObject chatChunkQ;
            ChatChunk::chatChunk2Json(chatChunkQ, conv.question);

            QJsonArray chatChunksA;
            for (auto answer : conv.answers) {
                QJsonObject chatchunkA;
                ChatChunk::chatChunk2Json(chatchunkA, answer);
                chatChunksA.append(chatchunkA);
            }

            QJsonObject convObj;
            convObj.insert("question", chatChunkQ);
            convObj.insert("answers", chatChunksA);

            convsArray.append(convObj);
        }

        QJsonDocument doc(convsArray);
        convsData = doc.toJson();
    }

    static void json2Convs(QVector<Conversations> &convs, const QByteArray &convsData)
    {
        if (convsData.isEmpty())
            return;

        QJsonArray convsArray = QJsonDocument::fromJson(convsData).array();
        for (auto conv : convsArray) {
            QJsonObject chatChunkQObj = conv.toObject().value("question").toObject();
            QJsonArray chatChunkAArray = conv.toObject().value("answers").toArray();

            ChatChunk chatchunkQ;
            ChatChunk::json2ChatChunk(QJsonDocument(chatChunkQObj).toJson(), chatchunkQ);

            QVector<ChatChunk> answers;
            for (auto answerObj : chatChunkAArray) {
                ChatChunk chatchunkA;
                ChatChunk::json2ChatChunk(QJsonDocument(answerObj.toObject()).toJson(), chatchunkA);

                //判断文生图是否存在，不存在则生成图片
//                if(chatchunkA.chatType == ChatText2Image){
//                    ChatChunk::chatImageBase64ToImage(chatchunkA);
//                }

                //判断模型图标是否存在，不存在则重新拷贝
                if(static_cast<LLMChatModel>(chatchunkA.llmModel) != NoModel){
                    QFile file(chatchunkA.llmIcon);
                    if(!file.exists()){
                        chatchunkA.llmIcon = LLMServerProxy::llmIcon(static_cast<LLMChatModel>(chatchunkA.llmModel));
                    }
                }

                answers.append(chatchunkA);
            }

            Conversations conversations;
            conversations.question = chatchunkQ;
            conversations.answers = answers;

            convs.append(conversations);
        }
    }

};
}

#endif // GLOBAL_H
