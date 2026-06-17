#include "global_key_define.h"
#include "messagenode.h"
#include "modeltool.h"

#include <QJsonArray>
#include <QReadLocker>
#include <QWriteLocker>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logConv)

using namespace uos_ai;

QJsonObject MessageNode::toJson() const
{
    QReadLocker locker(&m_lock);
    QJsonObject messageJson;
    messageJson[STR_KEY_ROLE] = role;
    messageJson[STR_KEY_MODEL_NAME] = modelName;
    messageJson[STR_KEY_MODEL_ID] = modelId;
    messageJson[STR_KEY_PREVIOUS] = previous;
    messageJson[STR_KEY_NEXT] = QJsonArray::fromStringList(next);
    messageJson[STR_KEY_CUR_NEXT] = curNext;

    QJsonArray renderArray;
    for (const RenderMessage &msg : render)
        renderArray.append(msg.toJson());

    messageJson[STR_KEY_RENDER_MESSAGE] = renderArray;

    QJsonArray modelArray;
    for (const ModelMessage &msg : message)
        modelArray.append(msg.toJson());
    messageJson[STR_KEY_MESSAGE] = modelArray;
    messageJson[STR_KEY_EXTENSION] = QJsonObject::fromVariantHash(extension);

    return messageJson;
}

QSharedPointer<MessageNode> MessageNode::fromJson(const QJsonObject &json)
{
    if (json.isEmpty())
        return QSharedPointer<MessageNode>();

    MessageNodePtr node(new MessageNode);

    // Parse role
    int roleValue = json.value(STR_KEY_ROLE).toInt();
    node->setRole(static_cast<MessageRole>(roleValue));

    // Parse modelId
    QString modelName = json.value(STR_KEY_MODEL_NAME).toString();
    node->setModelName(modelName);

    QString modelId = json.value(STR_KEY_MODEL_ID).toString();
    node->setModelId(modelId);

    // Parse previous
    QString previous = json.value(STR_KEY_PREVIOUS).toString();
    node->setPrevious(previous);

    // Parse next
    QStringList nextList;
    QJsonArray nextArray = json.value(STR_KEY_NEXT).toArray();
    for (const QJsonValue &val : nextArray) {
        nextList.append(val.toString());
    }
    node->setNext(nextList);

    // Parse curNext
    QString curNext = json.value(STR_KEY_CUR_NEXT).toString();
    node->setCurNext(curNext);

    // Parse render messages
    QJsonArray renderArray = json.value(STR_KEY_RENDER_MESSAGE).toArray();
    for (const QJsonValue &val : renderArray) {
        if (val.isObject()) {
            QJsonObject renderObj = val.toObject();
            RenderMessage renderMsg = renderMsg.fromJson(renderObj);
            node->appendRender(renderMsg);
        }
    }

    // Parse model messages
    QJsonArray messageArray = json.value(STR_KEY_MESSAGE).toArray();
    for (const QJsonValue &val : messageArray) {
        if (val.isObject()) {
            QJsonObject modelObj = val.toObject();
            ModelMessage modelMsg = ModelMessage::fromJson(modelObj);
            node->appendMessage(modelMsg);
        }
    }

    // Parse extension
    QJsonObject extensionObj = json.value(STR_KEY_EXTENSION).toObject();
    node->setExtension(extensionObj.toVariantHash());

    return node;
}

// Role
MessageRole MessageNode::getRole() const
{
    QReadLocker locker(&m_lock);
    return role;
}

void MessageNode::setRole(const MessageRole &role)
{
    QWriteLocker locker(&m_lock);
    this->role = role;
}

// Id
QString MessageNode::getId() const
{
    QReadLocker locker(&m_lock);
    return id;
}

void MessageNode::setId(const QString &id)
{
    QWriteLocker locker(&m_lock);
    this->id = id;
}

// ModelName
QString MessageNode::getModelName() const
{
    QReadLocker locker(&m_lock);
    return modelName;
}

void MessageNode::setModelName(const QString &modelName)
{
    QWriteLocker locker(&m_lock);
    this->modelName = modelName;
}

// ModelId
QString MessageNode::getModelId() const
{
    QReadLocker locker(&m_lock);
    return modelId;
}

void MessageNode::setModelId(const QString &modelId)
{
    QWriteLocker locker(&m_lock);
    this->modelId = modelId;
}

// Render
QList<RenderMessage> MessageNode::getRender() const
{
    QReadLocker locker(&m_lock);
    return render;
}

void MessageNode::setRender(const QList<RenderMessage> &render)
{
    QWriteLocker locker(&m_lock);
    this->render = render;
}

void MessageNode::appendRender(const RenderMessage &value)
{
    QWriteLocker locker(&m_lock);
    render.append(value);
}

// Message
QList<ModelMessage> MessageNode::getMessage() const
{
    QReadLocker locker(&m_lock);
    return message;
}

void MessageNode::setMessage(const QList<ModelMessage> &message)
{
    QWriteLocker locker(&m_lock);
    this->message = message;
}

void MessageNode::appendMessage(const ModelMessage &value)
{
    QWriteLocker locker(&m_lock);
    message.append(value);
}

// Previous
QString MessageNode::getPrevious() const
{
    QReadLocker locker(&m_lock);
    return previous;
}

void MessageNode::setPrevious(const QString &previous)
{
    QWriteLocker locker(&m_lock);
    this->previous = previous;
}

// Next
QStringList MessageNode::getNext() const
{
    QReadLocker locker(&m_lock);
    return next;
}

void MessageNode::setNext(const QStringList &next)
{
    QWriteLocker locker(&m_lock);
    this->next = next;
}

void MessageNode::appendNext(const QString &value)
{
    QWriteLocker locker(&m_lock);
    next.append(value);
}

// CurNext
QString MessageNode::getCurNext() const
{
    QReadLocker locker(&m_lock);
    return curNext;
}

void MessageNode::setCurNext(const QString &curNext)
{
    QWriteLocker locker(&m_lock);
    this->curNext = curNext;
}

// Extension
QVariantHash MessageNode::getExtension() const
{
    QReadLocker locker(&m_lock);
    return extension;
}

void MessageNode::setExtension(const QVariantHash &extension)
{
    QWriteLocker locker(&m_lock);
    this->extension = extension;
}

QJsonObject RenderMessage::toJson() const
{
    QJsonObject renderJson;
    renderJson[STR_KEY_TYPE] = GlobalUtil::contentTypeToString(type);

    QVariantHash dataHash = data.toHash();
    QJsonObject dataJson;
    switch (type) {
    case CntText:
    /*
    {
        "content": "文本内容"
    }
    */
        if (data.type() == QVariant::String) {
            dataJson[STR_KEY_CONTENT] = data.toString();
        }
        break;
    case CntImage:
    /*
    {
        "content": ["图片URL1", "图片URL2"]
    }
    */
        if (dataHash.contains(STR_KEY_CONTENT)) {
            dataJson[STR_KEY_CONTENT] = dataHash.value(STR_KEY_CONTENT).toJsonArray();
        }
        break;
    case CntFile:
    // 这个是用户侧气泡的内容
    /*
    {
        "type": 0,
        "index": 0,
        "filePath": "/path/to/file",
        "fileNameText": "filename.ext",
        "imgBase64": "base64_encoded_icon",
        "docContent": "document content",
        "isEnabledMouthOver": true,
        "isShowParsingStatus": true,
        "isParsingStatusEnd": false,
        "parsingStatusText": "Parsing...",
        "isFileParsingSuccess": false
    }
    */
        if (dataHash.contains(STR_KEY_TYPE)) {
            dataJson[STR_KEY_TYPE] = dataHash.value(STR_KEY_TYPE).toInt();
        }
        if (dataHash.contains(STR_KEY_INDEX)) {
            dataJson[STR_KEY_INDEX] = dataHash.value(STR_KEY_INDEX).toInt();
        }
        if (dataHash.contains(STR_KEY_FILE_PATH)) {
            dataJson[STR_KEY_FILE_PATH] = dataHash.value(STR_KEY_FILE_PATH).toString();
        }
        if (dataHash.contains(STR_KEY_FILE_NAME_TEXT)) {
            dataJson[STR_KEY_FILE_NAME_TEXT] = dataHash.value(STR_KEY_FILE_NAME_TEXT).toString();
        }
        if (dataHash.contains(STR_KEY_IMG_BASE64)) {
            dataJson[STR_KEY_IMG_BASE64] = dataHash.value(STR_KEY_IMG_BASE64).toString();
        }
        if (dataHash.contains(STR_KEY_DOC_CONTENT)) {
            dataJson[STR_KEY_DOC_CONTENT] = dataHash.value(STR_KEY_DOC_CONTENT).toString();
        }
        if (dataHash.contains(STR_KEY_IS_ENABLED_MOUTH_OVER)) {
            dataJson[STR_KEY_IS_ENABLED_MOUTH_OVER] = dataHash.value(STR_KEY_IS_ENABLED_MOUTH_OVER).toBool();
        }
        if (dataHash.contains(STR_KEY_IS_SHOW_PARSING_STATUS)) {
            dataJson[STR_KEY_IS_SHOW_PARSING_STATUS] = dataHash.value(STR_KEY_IS_SHOW_PARSING_STATUS).toBool();
        }
        if (dataHash.contains(STR_KEY_IS_PARSING_STATUS_END)) {
            dataJson[STR_KEY_IS_PARSING_STATUS_END] = dataHash.value(STR_KEY_IS_PARSING_STATUS_END).toBool();
        }
        if (dataHash.contains(STR_KEY_PARSING_STATUS_TEXT)) {
            dataJson[STR_KEY_PARSING_STATUS_TEXT] = dataHash.value(STR_KEY_PARSING_STATUS_TEXT).toString();
        }
        if (dataHash.contains(STR_KEY_IS_FILE_PARSING_SUCCESS)) {
            dataJson[STR_KEY_IS_FILE_PARSING_SUCCESS] = dataHash.value(STR_KEY_IS_FILE_PARSING_SUCCESS).toBool();
        }
        break;
    case CntTool:
    /*
    {
        "index": "id",
        "name": "工具名称",
        "status": 0,
        "params": "工具参数",
        "result": "工具执行结果"
    }
    */
        if (dataHash.contains(STR_KEY_INDEX)) {
            dataJson[STR_KEY_INDEX] = dataHash.value(STR_KEY_INDEX).toString();
        }
        if (dataHash.contains(STR_KEY_NAME)) {
            dataJson[STR_KEY_NAME] = dataHash.value(STR_KEY_NAME).toString();
        }
        if (dataHash.contains(STR_KEY_STATUS)) {
            dataJson[STR_KEY_STATUS] = dataHash.value(STR_KEY_STATUS).toInt();
        }
        if (dataHash.contains(STR_KEY_PARAMS)) {
            dataJson[STR_KEY_PARAMS] = dataHash.value(STR_KEY_PARAMS).toString();
        }
        if (dataHash.contains(STR_KEY_RESULT)) {
            dataJson[STR_KEY_RESULT] = dataHash.value(STR_KEY_RESULT).toString();
        }
        if (dataHash.contains(STR_KEY_DISPLAY_CONTENT)) {
            QVariant displayContentVar = dataHash.value(STR_KEY_DISPLAY_CONTENT);
            QVariantHash displayContentHash;
            if (displayContentVar.type() == QVariant::Hash) {
                displayContentHash = displayContentVar.toHash();
            } else if (displayContentVar.type() == QVariant::Map) {
                QVariantMap map = displayContentVar.toMap();
                for (auto it = map.constBegin(); it != map.constEnd(); ++it)
                    displayContentHash[it.key()] = it.value();
            }
            if (!displayContentHash.isEmpty())
                dataJson[STR_KEY_DISPLAY_CONTENT] = QJsonObject::fromVariantHash(displayContentHash);
        }
        break;
    case CntInstruction:
    /*
    {
        "reply_type" : "system_operation_switch", //响应类型： 系统操作卡片-开关、系统操作卡片-滑动、文字、空、商店应用列表（内容为数组，一个则直接展示下载卡片，多个则展示列表）、日程卡片
        "reply_content" : {}  // 响应内容：协议待定
    }
    */
        if (dataHash.contains(STR_KEY_REPLY_TYPE)) {
            dataJson[STR_KEY_REPLY_TYPE] = dataHash.value(STR_KEY_REPLY_TYPE).toString();
        }
        if (dataHash.contains(STR_KEY_REPLY_CONTENT)) {
            QVariant replyContentVar = dataHash.value(STR_KEY_REPLY_CONTENT);
            if (replyContentVar.type() == QVariant::Hash) {
                dataJson[STR_KEY_REPLY_CONTENT] = QJsonObject::fromVariantHash(replyContentVar.toHash());
            } else if (replyContentVar.type() == QVariant::List) {
                QVariantList replyContentList = replyContentVar.toList();
                QJsonArray replyContentArray;
                for (const QVariant &item : replyContentList) {
                    if (item.type() == QVariant::Hash) {
                        replyContentArray.append(QJsonObject::fromVariantHash(item.toHash()));
                    } else {
                        replyContentArray.append(QJsonValue::fromVariant(item));
                    }
                }
                dataJson[STR_KEY_REPLY_CONTENT] = replyContentArray;
            } else {
                dataJson[STR_KEY_REPLY_CONTENT] = QJsonValue::fromVariant(replyContentVar);
            }
        }
        break;
    case CntReasoning:
        if (data.type() == QVariant::String) {
            dataJson[STR_KEY_REASONING_CONTENT] = data.toString();
        } else if (dataHash.contains(STR_KEY_REASONING_CONTENT)) {
            dataJson[STR_KEY_REASONING_CONTENT] = dataHash.value(STR_KEY_REASONING_CONTENT).toString();
        }
        if (dataHash.contains(STR_KEY_STATUS)) {
            dataJson[STR_KEY_STATUS] = dataHash.value(STR_KEY_STATUS).toInt();
        }
        if (dataHash.contains(STR_KEY_ELAPSED)) {
            dataJson[STR_KEY_ELAPSED] = dataHash.value(STR_KEY_ELAPSED).toInt();
        }
        break;
    case CntWebSearch:
    /*
    {
        "title": "Online Search",
        // 0: searching, 1: reading, 2: completed, 3: failed
        "status": 0, // "searching" | "reading" | "completed" | "failed"
        "content": [
            {
                "url": "https://www.baidu.com",
                "title": "网页搜索标题"
            }
        ]
    }
    */
        if (dataHash.contains(STR_KEY_TITLE)) {
            dataJson[STR_KEY_TITLE] = dataHash.value(STR_KEY_TITLE).toString();
        }
        if (dataHash.contains(STR_KEY_STATUS)) {
            dataJson[STR_KEY_STATUS] = dataHash.value(STR_KEY_STATUS).toInt();
        }
        if (dataHash.contains(STR_KEY_CONTENT)) {
            QVariantList contentList = dataHash.value(STR_KEY_CONTENT).toList();
            QJsonArray contentArray;
            for (const QVariant &item : contentList) {
                QVariantHash itemHash;
                if (item.type() == QVariant::Hash)
                    itemHash = item.toHash();
                else if (item.type() == QVariant::Map) {
                    const auto map = item.toMap();
                    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
                        itemHash[it.key()] = it.value();
                }
                if (itemHash.isEmpty()) continue;
                QJsonObject itemJson;
                if (itemHash.contains(STR_KEY_URL))
                    itemJson[STR_KEY_URL] = itemHash.value(STR_KEY_URL).toString();
                if (itemHash.contains(STR_KEY_TITLE))
                    itemJson[STR_KEY_TITLE] = itemHash.value(STR_KEY_TITLE).toString();
                contentArray.append(itemJson);
            }
            dataJson[STR_KEY_CONTENT] = contentArray;
        }
        break;
    case CntIComps:
    /*
    {
        "id": "request_id",
        "ic_type": "bash_approve",
        "title": "...",
        ... (subtype-specific fields)
    }
    */
        dataJson = QJsonObject::fromVariantHash(dataHash);
        break;
    case CntAgentStep:
        if (dataHash.contains(STR_KEY_TITLE)) {
            dataJson[STR_KEY_TITLE] = dataHash.value(STR_KEY_TITLE).toString();
        }
        if (dataHash.contains(STR_KEY_STATUS)) {
            dataJson[STR_KEY_STATUS] = dataHash.value(STR_KEY_STATUS).toInt();
        }
        if (dataHash.contains(STR_KEY_CONTENT)) {
            dataJson[STR_KEY_CONTENT] = dataHash.value(STR_KEY_CONTENT).toString();
        }
        if (dataHash.contains(STR_KEY_ENTRIES)) {
            QVariantList entriesList = dataHash.value(STR_KEY_ENTRIES).toList();
            QJsonArray entriesArray;
            for (const QVariant &entry : entriesList) {
                QVariantHash entryHash;
                if (entry.type() == QVariant::Hash)
                    entryHash = entry.toHash();
                else if (entry.type() == QVariant::Map) {
                    const auto map = entry.toMap();
                    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
                        entryHash[it.key()] = it.value();
                }
                if (entryHash.isEmpty()) continue;
                QJsonObject entryJson;
                entryJson[STR_KEY_KIND] = entryHash.value(STR_KEY_KIND).toString();
                if (entryHash.value(STR_KEY_KIND).toString() == QLatin1String("text")) {
                    entryJson[STR_KEY_CONTENT] = entryHash.value(STR_KEY_CONTENT).toString();
                } else {
                    // kind == "tool": data 字段是 ToolUseData hash
                    QVariant dataVar = entryHash.value(STR_KEY_DATA);
                    QVariantHash toolHash;
                    if (dataVar.type() == QVariant::Hash)
                        toolHash = dataVar.toHash();
                    else if (dataVar.type() == QVariant::Map) {
                        const auto map = dataVar.toMap();
                        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
                            toolHash[it.key()] = it.value();
                    }
                    entryJson[STR_KEY_DATA] = QJsonObject::fromVariantHash(toolHash);
                }
                entriesArray.append(entryJson);
            }
            dataJson[STR_KEY_ENTRIES] = entriesArray;
        }
        break;
    case CntOutline:
    // 引用格式，仅存 id + title，完整大纲通过 getWorkspaceOutline 加载
        if (dataHash.contains(STR_KEY_ID)) {
            dataJson[STR_KEY_ID] = dataHash.value(STR_KEY_ID).toString();
        }
        if (dataHash.contains(STR_KEY_TITLE)) {
            dataJson[STR_KEY_TITLE] = dataHash.value(STR_KEY_TITLE).toString();
        }
        break;
    case CntDocCard:
    /*
    {
        "id": "文档ID",
        "title": "文档标题",
        "version": -1
    }
     */
        if (dataHash.contains(STR_KEY_ID)) {
            dataJson[STR_KEY_ID] = dataHash.value(STR_KEY_ID).toString();
        }
        if (dataHash.contains(STR_KEY_TITLE)) {
            dataJson[STR_KEY_TITLE] = dataHash.value(STR_KEY_TITLE).toString();
        }
        if (dataHash.contains(STR_KEY_VERSION)) {
            dataJson[STR_KEY_VERSION] = dataHash.value(STR_KEY_VERSION).toInt();
        }
        break;
    case CntCommandCard:
    /*
    {
        "cardType": "slider_card",  // 卡片类型字符串值
        "cardData": {},  // 卡片数据
        "toolName": "工具名称",
        "message": "操作结果消息",
        "errorCode": 0,  // 错误码
        "extraData": {}  // 额外数据
    }
     */
        if (dataHash.contains("cardType")) {
            // 保持cardType为字符串类型
            QVariant cardTypeVar = dataHash.value("cardType");
            if (cardTypeVar.type() == QVariant::String) {
                dataJson["cardType"] = cardTypeVar.toString();
            } else {
                // 如果是数字，转换为字符串
                dataJson["cardType"] = QString::number(cardTypeVar.toInt());
            }
        }
        if (dataHash.contains("cardData")) {
            QVariant cardDataVar = dataHash.value("cardData");
            if (cardDataVar.type() == QVariant::Hash) {
                dataJson["cardData"] = QJsonObject::fromVariantHash(cardDataVar.toHash());
            } else if (cardDataVar.type() == QVariant::Map) {
                QVariantMap map = cardDataVar.toMap();
                QJsonObject cardDataJson;
                for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                    cardDataJson[it.key()] = QJsonValue::fromVariant(it.value());
                }
                dataJson["cardData"] = cardDataJson;
            }
        }
        if (dataHash.contains("toolName")) {
            dataJson["toolName"] = dataHash.value("toolName").toString();
        }
        if (dataHash.contains("message")) {
            dataJson["message"] = dataHash.value("message").toString();
        }
        if (dataHash.contains("errorCode")) {
            dataJson["errorCode"] = dataHash.value("errorCode").toInt();
        }
        if (dataHash.contains("extraData")) {
            QVariant extraDataVar = dataHash.value("extraData");
            if (extraDataVar.type() == QVariant::Hash) {
                dataJson["extraData"] = QJsonObject::fromVariantHash(extraDataVar.toHash());
            } else if (extraDataVar.type() == QVariant::Map) {
                QVariantMap map = extraDataVar.toMap();
                QJsonObject extraDataJson;
                for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                    extraDataJson[it.key()] = QJsonValue::fromVariant(it.value());
                }
                dataJson["extraData"] = extraDataJson;
            }
        }
        break;
    case CntGuessYouWant:
    /*
    {
        "content": ["问题1", "问题2", "问题3"]
    }
     */
        if (dataHash.contains(STR_KEY_CONTENT)) {
            QVariant contentVar = dataHash.value(STR_KEY_CONTENT);
            if (contentVar.type() == QVariant::StringList) {
                dataJson[STR_KEY_CONTENT] = QJsonArray::fromStringList(contentVar.toStringList());
            } else if (contentVar.type() == QVariant::List) {
                QVariantList contentList = contentVar.toList();
                QJsonArray contentArray;
                for (const QVariant &item : contentList) {
                    contentArray.append(item.toString());
                }
                dataJson[STR_KEY_CONTENT] = contentArray;
            } else {
                dataJson[STR_KEY_CONTENT] = contentVar.toString();
            }
        }
        break;
    case CntError:
    {
        if (dataHash.contains(STR_KEY_ERROR)) {
            dataJson[STR_KEY_ERROR] = dataHash.value(STR_KEY_ERROR).toInt();
        }
        if (dataHash.contains(STR_KEY_ERROR_MESSAGE)) {
            dataJson[STR_KEY_ERROR_MESSAGE] = dataHash.value(STR_KEY_ERROR_MESSAGE).toString();
        }
        if (dataHash.contains(STR_KEY_HTTP_ERROR)) {
            dataJson[STR_KEY_HTTP_ERROR] = dataHash.value(STR_KEY_HTTP_ERROR).toInt();
        }
    }
    default:
        break;
    }

    renderJson[STR_KEY_DATA] = dataJson;
    return renderJson;
}

RenderMessage RenderMessage::fromJson(QJsonObject obj)
{
    RenderMessage renderMsg;

    // Parse type
    QString typeStr = obj.value(STR_KEY_TYPE).toString();
    renderMsg.type = GlobalUtil::contentTypeFromString(typeStr);

    // Parse data
    if (renderMsg.type == CntText) {
        // For text type, extract only the content string
        QJsonObject dataObj = obj.value(STR_KEY_DATA).toObject();
        renderMsg.data = dataObj.value(STR_KEY_CONTENT).toString();
    } else if (renderMsg.type == CntFile
               || renderMsg.type == CntImage
               || renderMsg.type == CntReasoning
               || renderMsg.type == CntWebSearch
               || renderMsg.type == CntAgentStep
               || renderMsg.type == CntTool
               || renderMsg.type == CntOutline
               || renderMsg.type == CntDocCard
               || renderMsg.type == CntCommandCard
               || renderMsg.type == CntError
               || renderMsg.type == CntIComps) {
        // For structured render types, extract the entire data object to preserve nested structures
        QJsonObject dataObj = obj.value(STR_KEY_DATA).toObject();
        QVariantHash variantData = dataObj.toVariantHash();
        renderMsg.data = variantData;
    } else {
        // For other types, extract the content field
        QJsonObject dataObj = obj.value(STR_KEY_DATA).toObject();
        renderMsg.data = dataObj.value(STR_KEY_CONTENT).toVariant().toHash();
    }


    return renderMsg;
}

RenderMessage RenderMessage::createTool(const QString &id, const QString &name, const QString &params, int status, const QString &result)
{
    RenderMessage toolMsg;

    toolMsg.type = CntTool;
    QVariantHash toolData;
    toolData[STR_KEY_INDEX] = id;
    toolData[STR_KEY_NAME] = name;
    toolData[STR_KEY_PARAMS] = params;
    toolData[STR_KEY_STATUS] = status;
    if (!result.isEmpty()) {
        toolData[STR_KEY_RESULT] = result;
    }

    toolMsg.data = toolData;

    return toolMsg;
}

QJsonObject ModelMessage::toJson() const
{
    QJsonObject modelJson;
    modelJson[STR_KEY_ROLE] = role;
    modelJson[STR_KEY_SOURCE] = source;

    QJsonArray contentArray;
    for (const MetaMessage &content : content) {
        QJsonObject contentJson;
        contentJson[STR_KEY_TYPE] = GlobalUtil::contentTypeToString(content.type);
        if (content.type == CntTool) {
            if (role.compare(STR_KEY_TOOL) == 0) {
                contentJson[STR_KEY_DATA] = QJsonObject::fromVariantHash(content.data.toHash());
            } else if (role.compare(STR_KEY_ASSISTANT) == 0){
                auto tools = content.data.value<ModelToolCallList>();
                QJsonArray tooolArray;
                for (const ModelToolCall &tool : tools) {
                    QJsonObject toolObj;
                    toolObj[STR_KEY_TOOL_CALL_ID] = tool.id;
                    toolObj[STR_KEY_NAME] = tool.name;
                    toolObj[STR_KEY_ARGUMENTS] = QJsonObject::fromVariantHash(tool.arguments);
                    tooolArray.append(toolObj);
                }
                contentJson[STR_KEY_DATA] = tooolArray;
            } else {
                qCWarning(logConv) << "Tool content type not supported for role:" << role;
                contentJson[STR_KEY_DATA] = content.data.toString();
            }
        } else {
            contentJson[STR_KEY_DATA] = content.data.toString();
        }

        contentArray.append(contentJson);
    }

    modelJson[STR_KEY_CONTENT] = contentArray;
    return modelJson;
}

ModelMessage ModelMessage::fromJson(QJsonObject obj)
{
    ModelMessage modelMsg;

    modelMsg.role = obj.value(STR_KEY_ROLE).toString();
    modelMsg.source = obj.value(STR_KEY_SOURCE).toString();

    QJsonArray contentArray = obj.value(STR_KEY_CONTENT).toArray();
    for (const QJsonValue &val : contentArray) {
        if (val.isObject()) {
            QJsonObject contentObj = val.toObject();
            MetaMessage metaMsg;

            QString typeStr = contentObj.value(STR_KEY_TYPE).toString();
            metaMsg.type = GlobalUtil::contentTypeFromString(typeStr);

            if (metaMsg.type == CntTool) {
                if (modelMsg.role.compare(STR_KEY_TOOL) == 0) {
                    QJsonObject dataObj = contentObj.value(STR_KEY_DATA).toObject();
                    metaMsg.data = dataObj.toVariantHash();
                } else if (modelMsg.role.compare(STR_KEY_ASSISTANT) == 0) {
                    QJsonArray toolArray = contentObj.value(STR_KEY_DATA).toArray();
                    ModelToolCallList tools;
                    for (const QJsonValue &toolVal : toolArray) {
                        if (toolVal.isObject()) {
                            QJsonObject toolObj = toolVal.toObject();
                            ModelToolCall tool;
                            tool.id = toolObj.value(STR_KEY_TOOL_CALL_ID).toString();
                            tool.name = toolObj.value(STR_KEY_NAME).toString();
                            tool.arguments = toolObj.value(STR_KEY_ARGUMENTS).toObject().toVariantHash();
                            tools.append(tool);
                        }
                    }
                    metaMsg.data = QVariant::fromValue(tools);
                } else {
                    qCWarning(logConv) << "Tool content type not supported for role:" << modelMsg.role;
                    metaMsg.data = contentObj.value(STR_KEY_DATA).toString();
                }
            } else {
                metaMsg.data = contentObj.value(STR_KEY_DATA).toString();
            }

            modelMsg.content.append(metaMsg);
        }
    }

    return modelMsg;
}

ModelMessage ModelMessage::toolOutput(const QString &id, const QString &content)
{
    ModelMessage toolMsg;

    toolMsg.role = STR_KEY_TOOL;
    MetaMessage toolContent;
    toolContent.type = CntTool;
    QVariantHash toolData;
    toolData[STR_KEY_TOOL_CALL_ID] = id;
    toolData[STR_KEY_CONTENT] = content;
    toolContent.data = toolData;

    toolMsg.content.append(toolContent);
    return toolMsg;
}

QList<MetaMessage> MetaMessage::fromHash(const QVariantHash &content)
{
    QList<MetaMessage> msgs;

    if (content.contains(STR_KEY_CONTENT)) {
        QString str = content.value(STR_KEY_CONTENT).toString();
        MetaMessage msg;
        msg.type = CntText;
        msg.data = str;

        msgs.append(msg);
    }

    if (content.contains(STR_KEY_REASONING_CONTENT)) {
        QString str = content.value(STR_KEY_REASONING_CONTENT).toString();
        MetaMessage msg;
        msg.type = CntReasoning;
        msg.data = str;

        msgs.append(msg);
    }

    if (content.contains(STR_KEY_TOOL_CALLS)) {
        auto tools = content.value(STR_KEY_TOOL_CALLS).value<ModelToolCallList>();

        MetaMessage msg;
        msg.type = CntTool;
        msg.data = QVariant::fromValue(tools);

        msgs.append(msg);
    }

    return msgs;
}
