#include "eaichatinfojsoncontrol.h"
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

EaiChatInfoJsonControl::EaiChatInfoJsonControl()
{
    auto dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_chatInfoPath = dirPath + "/ChatInfo.json";
    m_conversationInfoPath = dirPath + "/ConversationHistory.json";
#ifndef ENABLE_PERSONAL_KNOWLEDGE_ASSISTANT
    changeKnowledgeConvsInfoToUosAI();
#endif
}

EaiChatInfoJsonControl &EaiChatInfoJsonControl::localChatInfoJsonControl()
{
    static EaiChatInfoJsonControl control;
    return control;
}

void EaiChatInfoJsonControl::updateConvsInfo(const QString &assistantId, const QString &conversationId, const QString &assistantDisplayName, QVector<uos_ai::Conversations> conversations)
{
    // 1. 获取缓存目录路径
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // 2. 更新对话文件 --------------------------------------------
    QString conversationFilePath = dirPath + "/" + assistantId + "_" + conversationId + ".json";

    QJsonObject rootObj;
    rootObj["assistantId"] = assistantId;
    rootObj["conversationId"] = conversationId;

    QJsonArray convsArray;
    QByteArray convsData;
    uos_ai::Conversations::convs2Json(convsData, conversations);
    convsArray = QJsonDocument::fromJson(convsData).array();

    rootObj["conversations"] = convsArray;

    QJsonDocument jsonDoc(rootObj);

    QFile conversationfile(conversationFilePath);
    if (conversationfile.open(QIODevice::WriteOnly)) {
        conversationfile.write(jsonDoc.toJson());
        conversationfile.close();
        qCDebug(logAIGUI) << "Conversation file created successfully:" << conversationFilePath;
    } else {
        qCWarning(logAIGUI) << "Failed to create conversation file:" << conversationFilePath;
    }

    // 3. 更新ConversationHistory.json -----------
    QFile historyFile(m_conversationInfoPath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开历史文件:" << m_conversationInfoPath;
        qCWarning(logAIGUI) << "Failed to open history file:" << m_conversationInfoPath;
        return;
    }

    QByteArray historyData = historyFile.readAll();
    historyFile.close();

    QJsonDocument historyDoc = QJsonDocument::fromJson(historyData);
    if (historyDoc.isNull()) {
        qCWarning(logAIGUI) << "Invalid history file format";
        return;
    }

    QJsonObject historyObj = historyDoc.object();
    QString historyKey = assistantId + "_" + conversationId;

    if (historyObj.contains(historyKey)) {
        QJsonObject entry = historyObj[historyKey].toObject();
        entry["conversationTitle"] = conversations.first().question.displayContent;
        entry["conversationTimestamp"] = QDateTime::currentSecsSinceEpoch(); // 更新为当前时间戳
        entry["assistantDisplayName"] = assistantDisplayName;
        // 替换原来的条目
        historyObj[historyKey] = entry;
        qCDebug(logAIGUI) << "Updated timestamp for:" << historyKey;
    } else {
        qCWarning(logAIGUI) << "Conversation record not found:" << historyKey;
        return;
    }

    // 新增历史记录数量控制
    QVector<QPair<QString, qint64>> validEntries;
    for (auto it = historyObj.constBegin(); it != historyObj.constEnd(); ++it) {
        QJsonObject entry = it.value().toObject();
        if (!entry["conversationTitle"].toString().isEmpty()) {
            validEntries.append(qMakePair(it.key(), entry["conversationTimestamp"].toVariant().toLongLong()));
        }
    }

    // 当有效条目超过100时清理旧记录
    if (validEntries.size() > 100) {
        // 按时间戳升序排序
        std::sort(validEntries.begin(), validEntries.end(), 
            [](const QPair<QString, qint64> &a, const QPair<QString, qint64> &b) {
                return a.second < b.second;
            });

        // 计算需要删除的数量
        int removeCount = validEntries.size() - 100;
        for (int i = 0; i < removeCount; ++i) {
            historyObj.remove(validEntries[i].first);
            qCDebug(logAIGUI) << "Removed old record:" << validEntries[i].first;
        }
    }

    // 写回修改后的数据
    if (historyFile.open(QIODevice::WriteOnly)) {
        historyFile.write(QJsonDocument(historyObj).toJson());
        historyFile.close();
        qCDebug(logAIGUI) << "History file updated successfully";
    } else {
        qCWarning(logAIGUI) << "Failed to write history file";
    }
}

void EaiChatInfoJsonControl::getAllConvsInfo(QMap<QString, QJsonDocument> &assisConvs)
{
    QJsonArray convsArray;

    QFile file(m_conversationInfoPath);
    if (!file.open(QIODevice::ReadOnly)) {
        // 打开文件失败
        QJsonDocument jsonDocError(convsArray);
        qCWarning(logAIGUI) << "Failed to open conversation info file:" << m_conversationInfoPath;
        return;
    }
    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close();
    // 解析JSON
    QJsonParseError parseError;
    QJsonDocument chatInfoDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "JSON parse error:" << parseError.errorString();
        return;
    }

    if (!chatInfoDoc.isObject()) {
        qCWarning(logAIGUI) << "Root element is not an object";
        return;
    }

    QJsonObject jsonObject = chatInfoDoc.object();
    for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
        if (it.value().isObject()) {
            // 创建包含单个对象的文档
            QJsonObject convObj = it.value().toObject();
            QJsonDocument jsonDoc(convObj);
            assisConvs[it.key()] = jsonDoc;
        } else {
            qCWarning(logAIGUI) << "Value is not an object:" << it.key();
        }
    }
}

bool EaiChatInfoJsonControl::getConvsInfo(const QString &assistantId, const QString &conversationId, QVector<uos_ai::Conversations> &conversations)
{
    QJsonArray convsArray;

    // Get the directory path
    auto dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // Create the full file path (assuming the file is named "conversations.json")
    QString conversationFilePath = dirPath + "/" + assistantId + "_" + conversationId+".json";
    QFile file(conversationFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // 打开文件失败
        QJsonDocument jsonDocError(convsArray);
        qCWarning(logAIGUI) << "Failed to open conversation file:" << conversationFilePath;
        return false;
    }
    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close();
    QJsonDocument chatInfoDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = chatInfoDoc.object();
    QJsonDocument jsonDoc(jsonObject.find("conversations").value().toArray());
    uos_ai::Conversations::json2Convs(conversations, jsonDoc.toJson());

    // 时间戳更新
    QFile historyFile(m_conversationInfoPath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to open history file:" << m_conversationInfoPath;
        return false;
    }

    QByteArray historyData = historyFile.readAll();
    historyFile.close();

    QJsonDocument historyDoc = QJsonDocument::fromJson(historyData);
    if (historyDoc.isNull()) {
        qCWarning(logAIGUI) << "Invalid history file format";
        return false;
    }

    QJsonObject historyObj = historyDoc.object();

    QString historyKey = assistantId + "_" + conversationId;

    if (historyObj.contains(historyKey)) {
        QJsonObject entry = historyObj[historyKey].toObject();
        entry["accessTimestamp"] = QDateTime::currentSecsSinceEpoch(); // 更新为当前时间戳
        // 替换原来的条目
        historyObj[historyKey] = entry;
        qCDebug(logAIGUI) << "Updated access timestamp for:" << historyKey;
    } else {
        qCWarning(logAIGUI) << "Conversation record not found:" << historyKey;
    }

    if (historyFile.open(QIODevice::WriteOnly)) {
        historyFile.write(QJsonDocument(historyObj).toJson());
        historyFile.close();
        qCDebug(logAIGUI) << "History file updated successfully";
    } else {
        qCWarning(logAIGUI) << "Failed to write history file";
    }
    return true;
}

QString EaiChatInfoJsonControl::createConvs(const QString &assistantId, EAiExecutor::ConversionMode conversionMode)
{
    //清理旧的空对话
    QMap<QString, QJsonDocument> existingConvs;
    getAllConvsInfo(existingConvs);

    for (auto it = existingConvs.constBegin(); it != existingConvs.constEnd(); ++it) {
        QJsonObject conv = it.value().object();
        if (conv["assistantId"] == assistantId && conv["conversationTitle"].toString().isEmpty())
            removeConvs(assistantId, conv["conversationId"].toString());
    }

    // Current timestamp
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    QString conversationId = QString::number(timestamp);

    // Create the new conversation entry
    QJsonObject newConversation;
    newConversation["assistantId"] = assistantId;
    newConversation["conversationId"] = conversationId;
    newConversation["conversationTitle"] = "";
    newConversation["conversationTimestamp"] = 0;
    newConversation["assistantDisplayName"] = "";
    newConversation["accessTimestamp"] = timestamp;

    // Key for the JSON object (assistantId_conversationId)
    QString key = assistantId + "_" + conversationId;

    // Read existing data
    QJsonObject rootObject;
    QFile conversationInfofile(m_conversationInfoPath);

    if (conversationInfofile.exists() && conversationInfofile.open(QIODevice::ReadOnly)) {
        QByteArray data = conversationInfofile.readAll();
        conversationInfofile.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            rootObject = doc.object();
        }
    }
    rootObject[key] = newConversation;

    // Write back to file
    if (conversionMode == EAiExecutor::ConversionMode::Normal) {
        if (conversationInfofile.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(rootObject);
            conversationInfofile.write(doc.toJson());
            conversationInfofile.close();
            qCDebug(logAIGUI) << "Created new conversation with ID:" << conversationId;
        } else {
            qCWarning(logAIGUI) << "Failed to open conversation file for writing:" << m_conversationInfoPath;
        }
    }
    return conversationId;
}

void EaiChatInfoJsonControl::removeConvs(const QString &assistantId, const QString &conversationId)
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // 1. 读取现有的 ConversationHistory.json
    QFile historyFile(m_conversationInfoPath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to open history file:" << m_conversationInfoPath;
        return;
    }

    QByteArray historyData = historyFile.readAll();
    historyFile.close();

    QJsonDocument historyDoc = QJsonDocument::fromJson(historyData);
    if (historyDoc.isNull()) {
        qCWarning(logAIGUI) << "Invalid history file format";
        return;
    }

    QJsonObject historyObj = historyDoc.object();

    // 2. 构建要删除的键 (assistantId + "_" + conversationId)
    QString keyToDelete = assistantId + "_" + conversationId;

    // 3. 删除对应的项
    if (historyObj.contains(keyToDelete)) {
        historyObj.remove(keyToDelete);
        qCDebug(logAIGUI) << "Removed from history:" << keyToDelete;
    } else {
        qCWarning(logAIGUI) << "History record not found:" << keyToDelete;
    }

    // 4. 写回文件
    if (historyFile.open(QIODevice::WriteOnly)) {
        historyFile.write(QJsonDocument(historyObj).toJson());
        historyFile.close();
    } else {
        qCWarning(logAIGUI) << "Failed to write history file";
    }

    // 5. 删除对话文件
    QString conversationFilePath = dirPath + "/" + keyToDelete + ".json";
    QFile conversationFile(conversationFilePath);

    if (conversationFile.exists()) {
        if (conversationFile.remove()) {
            qCDebug(logAIGUI) << "Successfully removed conversation file:" << conversationFilePath;
        } else {
            qCWarning(logAIGUI) << "Failed to remove conversation file:" << conversationFile.errorString();
        }
    } else {
        qCWarning(logAIGUI) << "Conversation file does not exist:" << conversationFilePath;
    }
}

void EaiChatInfoJsonControl::removeAllConvs()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // 1. 清空ConversationHistory.json
    QFile historyFile(m_conversationInfoPath);
    if (historyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        historyFile.write(QJsonDocument(QJsonObject()).toJson());
        historyFile.close();
        qCDebug(logAIGUI) << "Cleared ConversationHistory.json";
    } else {
        qCWarning(logAIGUI) << "Failed to clear ConversationHistory.json";
    }

    // 2. 删除所有以"assistantId_conversationId.json"命名的会话文件
    QDir cacheDir(dirPath);
    QStringList convFiles = cacheDir.entryList(QStringList() << "*_*.json", QDir::Files);
    for (const QString &fileName : convFiles) {
        QString filePath = dirPath + "/" + fileName;
        QFile convFile(filePath);
        if (convFile.exists()) {
            if (convFile.remove()) {
                qCDebug(logAIGUI) << "Removed conversation file:" << filePath;
            } else {
                qCWarning(logAIGUI) << "Failed to remove conversation file:" << filePath;
            }
        }
    }
}

bool EaiChatInfoJsonControl::isConvsExist(const QString &assistantId, const QString &conversationId)
{
    QJsonArray convsArray;
    qCDebug(logAIGUI) << "Checking if conversation exists for assistant:" << assistantId
                      << "conversation:" << conversationId;

    QFile file(m_conversationInfoPath);
    if (!file.open(QIODevice::ReadOnly)) {
        // 打开文件失败
        QJsonDocument jsonDocError(convsArray);
        qCWarning(logAIGUI) << "Failed to open conversation info file:" << m_conversationInfoPath;
        return false;
    }
    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close();
    // 解析JSON
    QJsonDocument chatInfoDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = chatInfoDoc.object();
    QString conversationFileName = assistantId+"_"+conversationId;
    return jsonObject.find(conversationFileName) != jsonObject.constEnd();
}

QString EaiChatInfoJsonControl::findLatestConversation(const QString& targetAssistantId)
{
    qCDebug(logAIGUI) << "Finding latest conversation for assistant:" << targetAssistantId;

    QMap<QString, QJsonDocument> allConvs;
    getAllConvsInfo(allConvs);  // 获取全部会话信息

    QJsonDocument latestConversation;
    qint64 maxTimestamp = 0;

    QMapIterator<QString, QJsonDocument> it(allConvs);
    while (it.hasNext()) {
        it.next();
        QJsonObject convObj = it.value().object();
        if (convObj["assistantId"].toString() != targetAssistantId) continue;
        qint64 timestamp = convObj["accessTimestamp"].toVariant().toLongLong();
        if (timestamp > maxTimestamp) {
            maxTimestamp = timestamp;
            latestConversation = it.value();
        }
    }

    if (latestConversation.isEmpty()) {
        qCDebug(logAIGUI) << "No existing conversation found, creating new one";
        createConvs(targetAssistantId, EAiExecutor::ConversionMode::Normal);
        return findLatestConversation(targetAssistantId);
    }

    return latestConversation.toJson(QJsonDocument::Indented);
}

void EaiChatInfoJsonControl::changeKnowledgeConvsInfoToUosAI()
{
    qCDebug(logAIGUI) << "Starting to transfer personal-knowledge-assistant conversations to uos-assistant";
    
    // 读取ConversationHistory.json文件
    QFile historyFile(m_conversationInfoPath);
    if (!historyFile.open(QIODevice::ReadOnly)) {
        qCWarning(logAIGUI) << "Failed to open history file:" << m_conversationInfoPath;
        return;
    }
    
    QByteArray historyData = historyFile.readAll();
    historyFile.close();
    
    QJsonDocument historyDoc = QJsonDocument::fromJson(historyData);
    if (historyDoc.isNull()) {
        qCWarning(logAIGUI) << "Invalid history file format";
        return;
    }
    
    QJsonObject historyObj = historyDoc.object();
    QJsonObject newHistoryObj;
    int transferredCount = 0;
    
    // 遍历所有会话记录
    for (auto it = historyObj.constBegin(); it != historyObj.constEnd(); ++it) {
        QString key = it.key();
        QJsonObject entry = it.value().toObject();
        QString assistantId = entry["assistantId"].toString();
        
        // 检查是否是personal-knowledge-assistant的会话记录
        if (assistantId.contains("personal-knowledge-assistant") || 
            assistantId.contains("Personal Knowledge Assistant")) {
            
            // 获取缓存目录路径
            QString dirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            QString conversationFilePath = dirPath + "/" + key + ".json";
            
            // 读取会话文件内容
            QFile conversationFile(conversationFilePath);
            if (conversationFile.exists() && conversationFile.open(QIODevice::ReadOnly)) {
                QByteArray convData = conversationFile.readAll();
                conversationFile.close();
                
                QJsonDocument convDoc = QJsonDocument::fromJson(convData);
                if (!convDoc.isNull()) {
                    QJsonObject convObj = convDoc.object();
                    
                    // 保持原有的conversationId不变
                    QString originalConversationId = convObj["conversationId"].toString();
                    
                    // 使用固定的UOS AI助手ID
                    QString newUosAssistantId = "uos-ai";
                    QString newKey = newUosAssistantId + "_" + originalConversationId;
                    
                    // 更新会话文件中的assistantId
                    convObj["assistantId"] = newUosAssistantId;
                    
                    // 更新会话内容中的assistantId
                    QJsonArray conversationsArray = convObj["conversations"].toArray();
                    for (int i = 0; i < conversationsArray.size(); ++i) {
                        QJsonObject conv = conversationsArray[i].toObject();
                        
                        // 更新问题中的assistantId
                        if (conv.contains("question")) {
                            QJsonObject question = conv["question"].toObject();
                            question["assistantId"] = newUosAssistantId;
                            question["assistantName"] = "UOS AI";
                            conv["question"] = question;
                        }
                        
                        // 更新答案中的assistantId
                        if (conv.contains("answers")) {
                            QJsonArray answers = conv["answers"].toArray();
                            for (int j = 0; j < answers.size(); ++j) {
                                QJsonObject answer = answers[j].toObject();
                                answer["assistantId"] = newUosAssistantId;
                                answer["assistantName"] = "UOS AI";
                                answers[j] = answer;
                            }
                            conv["answers"] = answers;
                        }
                        
                        conversationsArray[i] = conv;
                    }
                    convObj["conversations"] = conversationsArray;
                    
                    // 保存新的会话文件
                    QString newConversationFilePath = dirPath + "/" + newKey + ".json";
                    QFile newConversationFile(newConversationFilePath);
                    if (newConversationFile.open(QIODevice::WriteOnly)) {
                        QJsonDocument newConvDoc(convObj);
                        newConversationFile.write(newConvDoc.toJson());
                        newConversationFile.close();
                        
                        // 创建新的历史记录条目，保持原有的时间戳
                        QJsonObject newEntry;
                        newEntry["assistantId"] = newUosAssistantId;
                        newEntry["conversationId"] = originalConversationId;
                        newEntry["conversationTitle"] = entry["conversationTitle"];
                        newEntry["conversationTimestamp"] = entry["conversationTimestamp"];
                        newEntry["assistantDisplayName"] = "UOS AI";
                        newEntry["accessTimestamp"] = entry["accessTimestamp"]; // 保持原有的访问时间戳
                        
                        newHistoryObj[newKey] = newEntry;
                        transferredCount++;
                        
                        qCDebug(logAIGUI) << "Transferred conversation:" << key << "to" << newKey;
                    } else {
                        qCWarning(logAIGUI) << "Failed to create new conversation file:" << newConversationFilePath;
                    }
                }
            }
            
            // 删除原始的personal-knowledge-assistant会话文件
            QFile::remove(conversationFilePath);
            qCDebug(logAIGUI) << "Removed original conversation file:" << conversationFilePath;
            
        } else {
            // 保留其他助手的会话记录
            newHistoryObj[key] = entry;
        }
    }
    
    // 写回更新后的历史记录文件
    if (historyFile.open(QIODevice::WriteOnly)) {
        QJsonDocument newHistoryDoc(newHistoryObj);
        historyFile.write(newHistoryDoc.toJson());
        historyFile.close();
        qCDebug(logAIGUI) << "Successfully updated history file with" << transferredCount << "transferred conversations";
    } else {
        qCWarning(logAIGUI) << "Failed to write updated history file";
        return;
    }
    
    qCInfo(logAIGUI) << QString("Successfully transferred %1 conversations from personal-knowledge-assistant to uos-ai").arg(transferredCount);
}
