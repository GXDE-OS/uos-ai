#include "conversationmigration.h"
#include "conversation/conversationmanager.h"
#include "conversation/conversationrecord.h"
#include "assistantmanager.h"
#include "global_define.h"
#include "agent/research/workspacestore.h"
#include "agent/research/writingworkspace.h"
#include "agent/research/article.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>

Q_DECLARE_LOGGING_CATEGORY(logMigration)

using namespace uos_ai;

QString ConversationMigration::name() const
{
    return "conversation_migration";
}

bool ConversationMigration::isNeeded() const
{
    // Check if ConversationHistory.json exists
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString historyFilePath = dirPath + "/ConversationHistory.json";
    return QFile::exists(historyFilePath);
}

bool ConversationMigration::migrate()
{
    return migrateConversationData();
}

// ChatAction to ContentType mapping
// ChatTextPlain = 0 → CntText (text)
// ChatFunctionCall = 1 → CntInstruction (instruction)
// ChatText2Image = 2 → CntImage (image)
// ChatTextThink = 3 → CntReasoning (thinking)
// ChatToolUse = 4 → CntTool (tool)
// AgentReasonTitle = 5 → CntAgentStep (agent_step)
// AgentReasoning = 6 → CntReasoning (agent_step)
// AgentAction = 7 → null
// ChatOutline = 8 → CntOutline (outline)
// ChatDocCard = 9 → CntDocCard (doc_card)
// ChatGuessYouWant = 10 → CntGuessYouWant (guess_you_want)
ContentType ConversationMigration::chatActionToContentType(int chatType)
{
    switch (chatType) {
        case 0: return CntText;
        case 1: return CntInstruction;
        case 2: return CntImage;
        case 3: return CntReasoning;
        case 4: return CntTool;
        case 5: return CntAgentStep;
        case 6: return CntAgentStep;
        case 7: return CntAgentStep;
        case 8: return CntOutline;
        case 9: return CntDocCard;
        case 10: return CntGuessYouWant;
        default: return CntText;
    }
}

// Parse displayContent JSON array and create render messages
// thinkTime is used for CntReasoning type to set elapsed field
// Note: AgentAction (chatType 7) is filtered out
//       AgentReasoning (chatType 6) content is merged into previous AgentReasonTitle's content field
QList<RenderMessage> ConversationMigration::parseDisplayContent(const QString &displayContent, int thinkTime)
{
    QList<RenderMessage> renderMessages;
    
    if (displayContent.isEmpty()) {
        return renderMessages;
    }
    
    // Try to parse as JSON array
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(displayContent.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        // Not a JSON array, treat as plain text
        RenderMessage textRender;
        textRender.type = CntText;
        // For CntText, data should be a string directly (not a hash)
        textRender.data = displayContent;
        renderMessages.append(textRender);
        return renderMessages;
    }
    
    QJsonArray contentArray = doc.array();
    for (const QJsonValue &val : contentArray) {
        if (!val.isObject())
            continue;
        
        QJsonObject contentObj = val.toObject();
        int chatType = contentObj.value("chatType").toInt(0);
        QString content = contentObj.value("content").toString();
        
        // Skip AgentAction (chatType 7) - filter it out completely
        if (chatType == 7) {
            continue;
        }
        
        // AgentReasoning (chatType 6): merge content into previous AgentReasonTitle's content field
        if (chatType == 6) {
            // Find the last CntAgentStep render message (should be from AgentReasonTitle)
            for (int i = renderMessages.size() - 1; i >= 0; --i) {
                if (renderMessages[i].type == CntAgentStep) {
                    QVariantHash dataHash = renderMessages[i].data.toHash();
                    // Merge AgentReasoning content into AgentReasonTitle's content field
                    QString existingContent = dataHash.value("content").toString();
                    if (existingContent.isEmpty()) {
                        dataHash["content"] = content;
                    } else {
                        dataHash["content"] = existingContent + "\n" + content;
                    }
                    renderMessages[i].data = dataHash;
                    break;
                }
            }
            continue;  // Don't create a new render message for AgentReasoning
        }
        
        RenderMessage renderMsg;
        renderMsg.type = chatActionToContentType(chatType);
        
        // Set data based on content type
        switch (renderMsg.type) {
        case CntText:
            // For CntText, data is a hash with "content" fie
            renderMsg.data = content;
            break;
        case CntReasoning:
            // For CntReasoning, data is a hash with reasoning_content, status, and elapsed
            {
                QVariantHash dataHash;
                dataHash["reasoning_content"] = content;
                dataHash["status"] = 1;  // Completed status
                dataHash["elapsed"] = thinkTime;
                renderMsg.data = dataHash;
            }
            break;
        case CntImage:
            {
                // For CntImage, data is a hash with "content" as image URLs array
                QVariantHash dataHash;
                QJsonArray imageUrls;
                // content might be comma-separated URLs or JSON array
                QJsonParseError imageParseError;
                QJsonDocument imageDoc = QJsonDocument::fromJson(content.toUtf8(), &imageParseError);
                if (imageParseError.error == QJsonParseError::NoError && imageDoc.isArray()) {
                    imageUrls = imageDoc.array();
                } else {
                    // Treat as single URL or comma-separated
                    QStringList urls = content.split(',', PARAM_SKIP_EMPTY);
                    for (const QString &url : urls) {
                        imageUrls.append(url.trimmed());
                    }
                }
                dataHash["content"] = imageUrls;
                renderMsg.data = dataHash;
            }
            break;
        case CntTool:
            {
                // For CntTool, data is a hash with tool info
                QVariantHash dataHash;
                dataHash["index"] = contentObj.value("index").toString();
                dataHash["name"] = contentObj.value("name").toString();
                dataHash["params"] = contentObj.value("params").toString();
                dataHash["result"] = contentObj.value("result").toString();
                dataHash["status"] = contentObj.value("status").toInt();
                renderMsg.data = dataHash;
            }
            break;
        case CntInstruction:
            {
                // For CntInstruction, data is a hash
                QVariantHash dataHash;
                dataHash["reply_type"] = contentObj.value("reply_type").toString();
                if (contentObj.contains("reply_content")) {
                    dataHash["reply_content"] = contentObj.value("reply_content").toVariant();
                }
                renderMsg.data = dataHash;
            }
            break;
        case CntAgentStep:
            {
                // For CntAgentStep (AgentReasonTitle, chatType 5 only now),
                // data is a hash with title, content, status (no entries)
                // Reference: src/agent/agentstep.h makeAgentStep()
                QVariantHash dataHash;
                
                // AgentReasonTitle: content is the title/status message
                dataHash["title"] = content;
                dataHash["content"] = "";  // Will be filled by subsequent AgentReasoning
                dataHash["status"] = contentObj.value("status").toInt(1);
                
                renderMsg.data = dataHash;
            }
            break;
        case CntOutline:
            {
                // For CntOutline, content is a JSON string containing nested outline structure
                // We need to parse it and extract id and title
                QVariantHash dataHash;
                
                // Try to parse content as JSON object
                QJsonParseError outlineParseError;
                QJsonDocument outlineDoc = QJsonDocument::fromJson(content.toUtf8(), &outlineParseError);
                if (outlineParseError.error == QJsonParseError::NoError && !outlineDoc.object().isEmpty()) {
                    QJsonObject outlineObj = outlineDoc.object();
                    // Extract title from the outline JSON
                    dataHash["title"] = outlineObj.value("title").toString();
                    // id might not exist in old format, generate a UUID
                    QString outlineId = contentObj.value("id").toString();
                    if (outlineId.isEmpty()) {
                        outlineId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    }
                    dataHash["id"] = outlineId;
                } else {
                    // Fallback: use content as title directly
                    dataHash["title"] = content;
                    dataHash["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
                }
                renderMsg.data = dataHash;
            }
            break;
        case CntDocCard:
            {
                // For CntDocCard, content is a JSON object with id, title, content fields
                // The content field in the old format can be either:
                // 1. A JSON object directly (not a string): {"id": "...", "title": "...", "content": "..."}
                // 2. A JSON string that needs parsing
                QVariantHash dataHash;
                
                // Check if content is a JSON object directly (not a string)
                if (contentObj.value("content").isObject()) {
                    QJsonObject docCardObj = contentObj.value("content").toObject();
                    dataHash["id"] = docCardObj.value("id").toString();
                    dataHash["title"] = docCardObj.value("title").toString();
                    dataHash["version"] = 1;
                } else {
                    // content is a string, try to parse as JSON
                    QJsonParseError docCardParseError;
                    QJsonDocument docCardDoc = QJsonDocument::fromJson(content.toUtf8(), &docCardParseError);
                    if (docCardParseError.error == QJsonParseError::NoError && docCardDoc.isObject()) {
                        QJsonObject docCardObj = docCardDoc.object();
                        dataHash["id"] = docCardObj.value("id").toString();
                        dataHash["title"] = docCardObj.value("title").toString();
                        dataHash["version"] = 1;
                    } else {
                        // Fallback: try to get from contentObj directly
                        dataHash["id"] = contentObj.value("id").toString();
                        dataHash["title"] = contentObj.value("title").toString();
                        dataHash["version"] = contentObj.value("version").toInt(1);
                    }
                }
                renderMsg.data = dataHash;
            }
            break;
        case CntGuessYouWant:
            {
                // For CntGuessYouWant, content is a JSON array of strings
                QVariantHash dataHash;
                QStringList questions;
                
                // content field might be a JSON array
                QJsonParseError guessParseError;
                QJsonDocument guessDoc = QJsonDocument::fromJson(content.toUtf8(), &guessParseError);
                if (guessParseError.error == QJsonParseError::NoError && guessDoc.isArray()) {
                    QJsonArray guessArray = guessDoc.array();
                    for (const QJsonValue &guessVal : guessArray) {
                        questions.append(guessVal.toString());
                    }
                } else {
                    // Fallback: try contentObj value if it's an array
                    if (contentObj.value("content").isArray()) {
                        QJsonArray guessArray = contentObj.value("content").toArray();
                        for (const QJsonValue &guessVal : guessArray) {
                            questions.append(guessVal.toString());
                        }
                    } else {
                        questions = content.split('\n', PARAM_SKIP_EMPTY);
                    }
                }
                dataHash["content"] = questions;
                renderMsg.data = dataHash;
            }
            break;
        default:
            // Default: store content as string
            renderMsg.data = content;
            break;
        }
        
        renderMessages.append(renderMsg);
    }
    
    return renderMessages;
}

// Parse content for model messages (similar to displayContent but simpler)
QList<MetaMessage> ConversationMigration::parseModelContent(const QString &displayContent)
{
    QList<MetaMessage> metaMessages;
    
    if (displayContent.isEmpty()) {
        return metaMessages;
    }
    
    // Try to parse as JSON array
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(displayContent.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        // Not a JSON array, treat as plain text
        MetaMessage textMeta;
        textMeta.type = CntText;
        textMeta.data = displayContent;
        metaMessages.append(textMeta);
        return metaMessages;
    }
    
    QJsonArray contentArray = doc.array();
    for (const QJsonValue &val : contentArray) {
        if (!val.isObject())
            continue;
        
        QJsonObject contentObj = val.toObject();
        int chatType = contentObj.value("chatType").toInt(0);
        QString content = contentObj.value("content").toString();
        
        MetaMessage metaMsg;
        metaMsg.type = chatActionToContentType(chatType);
        metaMsg.data = content;
        metaMessages.append(metaMsg);
    }
    
    return metaMessages;
}

bool ConversationMigration::importFromOldFormat(const QString &fileName)
{
    // Get the cache location
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString filePath = dirPath + "/" + fileName;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(logMigration) << "Old format file not found, skipping import:" << filePath;
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logMigration) << "JSON parse error:" << parseError.errorString();
        return false;
    }
    
    QJsonObject rootObj = doc.object();
    
    // Extract assistantId and conversationId from the file
    QString assistantId = rootObj.value("assistantId").toString();
    QString conversationId = rootObj.value("conversationId").toString();
    
    if (assistantId.isEmpty() || conversationId.isEmpty()) {
        qCWarning(logMigration) << "Invalid file format: missing assistantId or conversationId";
        return false;
    }

    if (assistantId == "uos-ai" || assistantId == "ai-text-processing") {
        assistantId = STR_KEY_UOS_AI_GENERIC;
    } else if (assistantId == "ai-writing") {
        assistantId = STR_KEY_UOS_AI_WRITING;
    } else if (assistantId == "ai-translation") {
        assistantId = STR_KEY_UOS_AI_TRANSLATION;
    } else if (assistantId == "personal-knowledge-assistant") {
        assistantId = STR_KEY_UOS_AI_KNOWLEDGE_BASE;
    }

    // Parse conversations array
    QJsonArray conversationsArray = rootObj.value("conversations").toArray();
    if (conversationsArray.isEmpty()) {
        qCWarning(logMigration) << "No conversations found in file";
        return false;
    }
    
    // Convert old format to QVector<OldConversations>
    QVector<uos_ai::OldConversations> conversations;
    uos_ai::OldConversations::json2Convs(conversations, QJsonDocument(conversationsArray).toJson());
    
    if (conversations.isEmpty()) {
        qCWarning(logMigration) << "Failed to parse conversations";
        return false;
    }
    
    // Check if the conversation file already exists
    ConversationManager *convMgr = ConversationManager::instance();
    QString newFilePath = convMgr->getConversationFilePath(conversationId);
    if (QFile::exists(newFilePath)) {
        qCDebug(logMigration) << "Conversation already imported, skipping:" << conversationId;
        return true;  // Already imported
    }
    
    // Create a new ConversationRecord with the conversationId
    ConversationRecordPtr record(new ConversationRecord(conversationId));
    record->setAssistantId(assistantId);
    
    // Set title from first question's displayContent
    if (!conversations.isEmpty() && !conversations.first().question.displayContent.isEmpty()) {
        // Extract just the text content for title
        QString title = conversations.first().question.displayContent;
        // Try to parse as JSON and get first text content
        QJsonParseError titleParseError;
        QJsonDocument titleDoc = QJsonDocument::fromJson(title.toUtf8(), &titleParseError);
        if (titleParseError.error == QJsonParseError::NoError && titleDoc.isArray()) {
            QJsonArray titleArray = titleDoc.array();
            for (const QJsonValue &val : titleArray) {
                if (val.isObject()) {
                    QJsonObject titleObj = val.toObject();
                    if (titleObj.value("chatType").toInt(0) == 0) {
                        title = titleObj.value("content").toString();
                        break;
                    }
                }
            }
        }
        record->setTitle(title);
    }
    
    // Store all answer IDs for each question to set next pointers
    QVector<QString> prevQuestionAnswerIds;  // Answer IDs from previous question
    
    // Store question nodes for setting cur_next later
    QVector<QString> questionIds;
    
    // Collect outline data for workspace creation
    struct OutlineData {
        QString articleId;
        QString title;
        QJsonObject outlineJson;
        QString nodeId;  // The answer node ID that contains this outline
    };
    QVector<OutlineData> outlinesToCreate;
    
    // Collect doc card data for workspace creation
    struct DocCardData {
        QString articleId;
        QString title;
        QString content;  // The article content
        QString nodeId;   // The answer node ID that contains this doc card
    };
    QVector<DocCardData> docCardsToCreate;
    
    // Process conversations and build message nodes
    for (int i = 0; i < conversations.size(); ++i) {
        const uos_ai::OldConversations &conv = conversations[i];
        
        // Create question node
        MessageNodePtr questionNode(new MessageNode());
        QString questionId = GlobalUtil::generateUuid();
        questionNode->setId(questionId);
        questionNode->setRole(MrUser);
        questionNode->setModelName(conv.question.assistantName);
        questionNode->setModelId(conv.question.llmId);
        
        // Parse displayContent for render messages
        QList<RenderMessage> questionRenderMessages = parseDisplayContent(conv.question.displayContent);
        for (const RenderMessage &renderMsg : questionRenderMessages) {
            questionNode->appendRender(renderMsg);
        }
        
        // Create model message for question content
        ModelMessage questionModelMsg;
        questionModelMsg.role = "user";
        questionModelMsg.source = conv.question.assistantName;
        
        QList<MetaMessage> questionMetaMessages = parseModelContent(conv.question.displayContent);
        if (questionMetaMessages.isEmpty()) {
            // Fallback to simple text
            MetaMessage questionMeta;
            questionMeta.type = CntText;
            questionMeta.data = conv.question.content.isEmpty() ? conv.question.displayContent : conv.question.content;
            questionModelMsg.content.append(questionMeta);
        } else {
            for (const MetaMessage &metaMsg : questionMetaMessages) {
                questionModelMsg.content.append(metaMsg);
            }
        }
        questionNode->appendMessage(questionModelMsg);
        
        // Determine previous node for this question
        QString previousId;
        if (i == 0) {
            // First question: previous is root (empty string in addMessage)
            previousId = "";
        } else if (!prevQuestionAnswerIds.isEmpty()) {
            // Subsequent questions: previous is the last answer of previous conversation
            previousId = prevQuestionAnswerIds.last();
        }
        
        // Add question to record
        record->addMessage(previousId, questionNode);
        questionIds.append(questionId);
        
        // Process answers for this question
        QVector<QString> currentAnswerIds;
        for (int j = 0; j < conv.answers.size(); ++j) {
            const uos_ai::OldChatChunk &answerChunk = conv.answers[j];
            
            // Create answer node
            MessageNodePtr answerNode(new MessageNode());
            QString answerId = GlobalUtil::generateUuid();
            answerNode->setId(answerId);
            answerNode->setRole(MrAssistant);
            answerNode->setModelName(answerChunk.llmName);
            answerNode->setModelId(answerChunk.llmId);
            
            // Get thinkTime from answer chunk (convert to int)
            int thinkTime = 0;
            if (!answerChunk.thinkTime.isEmpty()) {
                thinkTime = answerChunk.thinkTime.toInt();
            }
            
            // Parse displayContent for render messages, pass thinkTime for reasoning elapsed
            QList<RenderMessage> answerRenderMessages = parseDisplayContent(answerChunk.displayContent, thinkTime);
            for (const RenderMessage &renderMsg : answerRenderMessages) {
                answerNode->appendRender(renderMsg);
                
                // Collect outline data for workspace creation
                if (renderMsg.type == CntOutline) {
                    OutlineData outlineData;
                    outlineData.nodeId = answerId;  // Store the node ID for later update
                    QVariantHash dataHash = renderMsg.data.toHash();
                    outlineData.articleId = dataHash.value("id").toString();
                    outlineData.title = dataHash.value("title").toString();
                    
                    // Parse the original outline JSON from displayContent
                    // Need to find the original content string
                    QJsonParseError outlineParseError;
                    QJsonDocument outlineDoc = QJsonDocument::fromJson(answerChunk.displayContent.toUtf8(), &outlineParseError);
                    if (outlineParseError.error == QJsonParseError::NoError && outlineDoc.isArray()) {
                        QJsonArray contentArray = outlineDoc.array();
                        for (const QJsonValue &val : contentArray) {
                            if (val.isObject()) {
                                QJsonObject contentObj = val.toObject();
                                if (contentObj.value("chatType").toInt(0) == 8) {
                                    QString content = contentObj.value("content").toString();
                                    QJsonParseError innerParseError;
                                    QJsonDocument innerDoc = QJsonDocument::fromJson(content.toUtf8(), &innerParseError);
                                    if (innerParseError.error == QJsonParseError::NoError && !innerDoc.object().isEmpty()) {
                                        outlineData.outlineJson = innerDoc.object();
                                        // Convert to workspace format: { "title": "...", "content": [...] }
                                        // The old format might use different key names
                                        if (outlineData.outlineJson.contains("content")) {
                                            // Already in correct format
                                        } else if (outlineData.outlineJson.contains("paragraphs")) {
                                            // Convert paragraphs to content
                                            QJsonArray paragraphs = outlineData.outlineJson.value("paragraphs").toArray();
                                            outlineData.outlineJson["content"] = paragraphs;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (!outlineData.articleId.isEmpty() && !outlineData.title.isEmpty()) {
                        outlinesToCreate.append(outlineData);
                    }
                }
                
                // Collect doc card data for workspace creation
                if (renderMsg.type == CntDocCard) {
                    DocCardData docCardData;
                    docCardData.nodeId = answerId;
                    QVariantHash dataHash = renderMsg.data.toHash();
                    docCardData.articleId = dataHash.value("id").toString();
                    docCardData.title = dataHash.value("title").toString();
                    
                    // Parse the original doc card JSON from displayContent to get content
                    QJsonParseError docCardParseError;
                    QJsonDocument docCardDoc = QJsonDocument::fromJson(answerChunk.displayContent.toUtf8(), &docCardParseError);
                    if (docCardParseError.error == QJsonParseError::NoError && docCardDoc.isArray()) {
                        QJsonArray contentArray = docCardDoc.array();
                        for (const QJsonValue &val : contentArray) {
                            if (val.isObject()) {
                                QJsonObject contentObj = val.toObject();
                                if (contentObj.value("chatType").toInt(0) == 9) {
                                    // content field is a JSON object
                                    if (contentObj.value("content").isObject()) {
                                        QJsonObject docObj = contentObj.value("content").toObject();
                                        docCardData.content = docObj.value("content").toString();
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (!docCardData.articleId.isEmpty() && !docCardData.title.isEmpty()) {
                        docCardsToCreate.append(docCardData);
                    }
                }
            }
            
            // Create model message for answer
            ModelMessage answerModelMsg;
            answerModelMsg.role = "assistant";
            answerModelMsg.source = answerChunk.llmName;
            
            QList<MetaMessage> answerMetaMessages = parseModelContent(answerChunk.displayContent);
            if (answerMetaMessages.isEmpty()) {
                // Fallback to simple text
                MetaMessage answerMeta;
                answerMeta.type = CntText;
                answerMeta.data = answerChunk.content.isEmpty() ? answerChunk.displayContent : answerChunk.content;
                answerModelMsg.content.append(answerMeta);
            } else {
                for (const MetaMessage &metaMsg : answerMetaMessages) {
                    answerModelMsg.content.append(metaMsg);
                }
            }
            answerNode->appendMessage(answerModelMsg);
            
            // Add answer to record with question as previous
            record->addMessage(questionId, answerNode);
            
            // Store answer ID
            currentAnswerIds.append(answerId);
        }
        
        // Now set up the next pointers for the question
        // question's next contains all answer IDs
        for (const QString &answerId : currentAnswerIds) {
            questionNode->appendNext(answerId);
        }
        
        // Set cur_next for question: first answer's id
        if (!currentAnswerIds.isEmpty()) {
            questionNode->setCurNext(currentAnswerIds.first());
        }
        
        // Set next and cur_next for answers
        for (int j = 0; j < currentAnswerIds.size(); ++j) {
            MessageNodePtr answerNode = record->messageAt(currentAnswerIds[j]);
            if (!answerNode)
                continue;
            
            // Each answer's next points to the next answer's id
            if (j < currentAnswerIds.size() - 1) {
                // Not the last answer: set next to point to the next answer
                answerNode->appendNext(currentAnswerIds[j + 1]);
            }
        }
        
        // Store current answer IDs for next iteration
        prevQuestionAnswerIds = currentAnswerIds;
    }
    
    // Set cur_next for the last answer of each question to point to next question
    for (int i = 0; i < questionIds.size() - 1; ++i) {
        // Get the question node to find its answers
        MessageNodePtr questionNode = record->messageAt(questionIds[i]);
        if (!questionNode || questionNode->getNext().isEmpty())
            continue;
        
        // Get the last answer of this question
        QString lastAnswerId = questionNode->getNext().last();
        MessageNodePtr lastAnswer = record->messageAt(lastAnswerId);
        if (lastAnswer) {
            // Set cur_next to point to next question
            lastAnswer->setCurNext(questionIds[i + 1]);
        }
    }
    
    // Generate introduction from last answer's text content
    if (!conversations.isEmpty() && !prevQuestionAnswerIds.isEmpty()) {
        MessageNodePtr lastAnswer = record->messageAt(prevQuestionAnswerIds.last());
        if (lastAnswer) {
            QList<RenderMessage> renders = lastAnswer->getRender();
            for (const RenderMessage &render : renders) {
                if (render.type == CntText) {
                    QString intro = render.data.toString();
                    if (!intro.isEmpty()) {
                        record->setIntroduction(intro);
                        break;
                    }
                }
            }
        }
    }
    
    // Create workspace for ai-writing assistant if there are outlines or doc cards
    // Note: This must be done BEFORE saving the record so that we can update
    // the render message IDs to match the actual article IDs
    if (assistantId == STR_KEY_UOS_AI_WRITING && (!outlinesToCreate.isEmpty() || !docCardsToCreate.isEmpty())) {
        // Create new workspace (we already checked that conversation doesn't exist,
        // so workspace should also not exist)
        WritingWorkspace ws;
        ws.setConversationId(conversationId);
        
        // Map from old articleId to new actual articleId
        QMap<QString, QString> articleIdMap;
        
        // Create articles from outlines
        for (const OutlineData &outlineData : outlinesToCreate) {
            QString actualArticleId = ws.addArticle(outlineData.title);
            articleIdMap[outlineData.articleId] = actualArticleId;
            
            Article *art = ws.article(actualArticleId);
            if (art && !outlineData.outlineJson.isEmpty()) {
                art->setOutline(outlineData.outlineJson);
            }
            
            qCDebug(logMigration) << "Created article from outline:" << actualArticleId
                             << "for outline:" << outlineData.articleId
                             << "title:" << outlineData.title;
        }
        
        // Create articles from doc cards
        for (const DocCardData &docCardData : docCardsToCreate) {
            QString actualArticleId = ws.addArticle(docCardData.title);
            articleIdMap[docCardData.articleId] = actualArticleId;
            
            Article *art = ws.article(actualArticleId);
            if (art) {
                // Save the article content
                if (!docCardData.content.isEmpty()) {
                    art->updateContent(docCardData.content);
                }
            }
            
            qCDebug(logMigration) << "Created article from doc card:" << actualArticleId
                             << "for doc card:" << docCardData.articleId
                             << "title:" << docCardData.title;
        }
        
        // Update render messages with actual article IDs using stored node IDs
        for (const OutlineData &outlineData : outlinesToCreate) {
            if (outlineData.nodeId.isEmpty())
                continue;
            
            MessageNodePtr node = record->messageAt(outlineData.nodeId);
            if (!node)
                continue;
            
            QList<RenderMessage> renders = node->getRender();
            bool modified = false;
            for (int k = 0; k < renders.size(); ++k) {
                if (renders[k].type == CntOutline) {
                    QVariantHash dataHash = renders[k].data.toHash();
                    QString oldId = dataHash.value("id").toString();
                    if (articleIdMap.contains(oldId)) {
                        dataHash["id"] = articleIdMap[oldId];
                        renders[k].data = dataHash;
                        modified = true;
                        qCDebug(logMigration) << "Updated outline render message ID from" << oldId
                                         << "to" << articleIdMap[oldId];
                    } else {
                        qCWarning(logMigration) << "articleIdMap does not contain oldId:" << oldId
                                           << "available keys:" << articleIdMap.keys();
                    }
                }
            }
            
            if (modified) {
                // Update the render messages in the node
                node->setRender(renders);
            }
        }
        
        // Update doc card render messages with actual article IDs
        for (const DocCardData &docCardData : docCardsToCreate) {
            if (docCardData.nodeId.isEmpty())
                continue;
            
            MessageNodePtr node = record->messageAt(docCardData.nodeId);
            if (!node)
                continue;
            
            QList<RenderMessage> renders = node->getRender();
            bool modified = false;
            for (int k = 0; k < renders.size(); ++k) {
                if (renders[k].type == CntDocCard) {
                    QVariantHash dataHash = renders[k].data.toHash();
                    QString oldId = dataHash.value("id").toString();
                    if (articleIdMap.contains(oldId)) {
                        dataHash["id"] = articleIdMap[oldId];
                        renders[k].data = dataHash;
                        modified = true;
                        qCDebug(logMigration) << "Updated doc card render message ID from" << oldId
                                         << "to" << articleIdMap[oldId];
                    } else {
                        qCWarning(logMigration) << "articleIdMap does not contain doc card oldId:" << oldId
                                           << "available keys:" << articleIdMap.keys();
                    }
                }
            }
            
            if (modified) {
                // Update the render messages in the node
                node->setRender(renders);
            }
        }
        
        // Save the workspace
        if (WorkspaceStore::instance()->save(ws)) {
            qCDebug(logMigration) << "Successfully created workspace for conversation:" << conversationId;
        } else {
            qCWarning(logMigration) << "Failed to save workspace for conversation:" << conversationId;
        }
    }
    
    // Save the converted record using ConversationManager
    bool saved = record->saveToFile(newFilePath) && convMgr->addOrUpdateIndex(record);
    if (saved) {
        qCDebug(logMigration) << "Successfully imported conversation from old format:" << conversationId;
    } else {
        qCWarning(logMigration) << "Failed to save imported conversation:" << conversationId;
    }
    
    return saved;
}

void ConversationMigration::importAllFromOldFormat()
{
    // Get the cache location
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString historyFilePath = dirPath + "/ConversationHistory.json";
    
    QFile historyFile(historyFilePath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qCDebug(logMigration) << "ConversationHistory.json not found, skipping bulk import";
        return;
    }
    
    QByteArray historyData = historyFile.readAll();
    historyFile.close();
    
    QJsonParseError parseError;
    QJsonDocument historyDoc = QJsonDocument::fromJson(historyData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logMigration) << "Failed to parse ConversationHistory.json:" << parseError.errorString();
        return;
    }
    
    QJsonObject historyObj = historyDoc.object();
    QStringList keys = historyObj.keys();
    
    if (keys.isEmpty()) {
        qCDebug(logMigration) << "No conversations found in ConversationHistory.json";
        // Remove the empty history file
        QFile::remove(historyFilePath);
        return;
    }
    
    qCDebug(logMigration) << "Found" << keys.size() << "conversations to import";
    
    QStringList successfullyImported;
    
    for (const QString &key : keys) {
        // Key format: "assistantId_conversationId"
        // File name format: "{key}.json"
        QString fileName = key + ".json";
        QString oldFilePath = dirPath + "/" + fileName;
        
        if (!QFile::exists(oldFilePath)) {
            qCDebug(logMigration) << "Old conversation file not found, skipping:" << oldFilePath;
            continue;
        }
        
        if (importFromOldFormat(fileName)) {
            successfullyImported.append(key);
            qCDebug(logMigration) << "Successfully imported:" << key;
        } else {
            qCWarning(logMigration) << "Failed to import:" << key;
        }
    }
    
    // Delete successfully imported old files
    for (const QString &key : successfullyImported) {
        QString oldFilePath = dirPath + "/" + key + ".json";
        if (QFile::exists(oldFilePath)) {
            if (QFile::remove(oldFilePath)) {
                qCDebug(logMigration) << "Deleted old conversation file:" << oldFilePath;
            } else {
                qCWarning(logMigration) << "Failed to delete old conversation file:" << oldFilePath;
            }
        }
    }
    
    // Delete ConversationHistory.json
    if (QFile::remove(historyFilePath)) {
        qCDebug(logMigration) << "Deleted ConversationHistory.json";
    } else {
        qCWarning(logMigration) << "Failed to delete ConversationHistory.json";
    }
    
    qCDebug(logMigration) << "Bulk import completed. Imported" << successfullyImported.size()
                     << "of" << keys.size() << "conversations";
}

bool ConversationMigration::migrateConversationData()
{
    importAllFromOldFormat();
    qCInfo(logMigration) << "Conversation migration completed";
    return true;
}
