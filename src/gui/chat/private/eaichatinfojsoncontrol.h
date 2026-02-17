#ifndef EAICHATINFOJSONCONTROL_H
#define EAICHATINFOJSONCONTROL_H

#include <QObject>
#include "global.h"
#include "eaiexecutor.h"

class EaiChatInfoJsonControl
{
public:
    EaiChatInfoJsonControl();

    static EaiChatInfoJsonControl &localChatInfoJsonControl();

    //查询所有会话记录
    void getAllConvsInfo(QMap<QString, QJsonDocument> &assisConvs);

    //读取某个会话记录
    bool getConvsInfo(const QString &assistantId, const QString &conversationId, QVector<uos_ai::Conversations> &conversations);

    //新建会话记录
    QString createConvs(const QString &assistantId, EAiExecutor::ConversionMode conversionMode);

    //删除会话记录
    void removeConvs(const QString &assistantId, const QString &conversationId);

    //删除全部会话记录
    void removeAllConvs();

    //更新会话记录
    void updateConvsInfo(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, QVector<uos_ai::Conversations> conversations);

    //会话记录是否存在
    bool isConvsExist(const QString &assistantId, const QString &conversationId);

    //获取助手最后一次访问的会话id
    QString findLatestConversation(const QString& targetAssistantId);

    //将personal-knowledge-assistant的会话记录转给uos-ai-assistant
    void changeKnowledgeConvsInfoToUosAI();

private:
    QString m_chatInfoPath;
    QString m_conversationInfoPath;
};

#endif // EAICHATINFOJSONCONTROL_H
