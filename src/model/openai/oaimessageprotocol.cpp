#include "oaimessageprotocol.h"
#include "global_key_define.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpressionMatchIterator>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

OaiMessageProtocol::OaiMessageProtocol()
{
}

OaiMessageProtocol::~OaiMessageProtocol()
{
}

void OaiMessageProtocol::initMessage(const QList<ModelMessage> &messages)
{
    m_messages = QJsonArray();
    for (const ModelMessage &msg : messages)
        addMessage(msg);
}

void OaiMessageProtocol::addMessage(const ModelMessage &msg)
{
    if (QString(STR_KEY_SYSTEM).compare(msg.role) == 0) {
        if (!m_messages.isEmpty()) {
            QString frole = m_messages.first().toObject()[STR_KEY_ROLE].toString();
            if (frole.compare(STR_KEY_SYSTEM) == 0) {
                m_messages.removeFirst();
            }
        }
        auto tmp = buildMessage(msg);
        if (!tmp.isEmpty())
            m_messages.prepend(tmp);
    } else {
        auto tmp = buildMessage(msg);
        if (!tmp.isEmpty())
            m_messages.append(tmp);
    }
}

void OaiMessageProtocol::addTool(const ModelTool &tool)
{
    QJsonObject toolObj;
    toolObj.insert(STR_KEY_NAME, sanitizeToolName(tool.name));
    toolObj.insert(STR_KEY_DESCRIPTION, tool.description);

    QJsonObject parametersObj;
    parametersObj.insert(STR_KEY_TYPE, "object");
    parametersObj.insert(STR_KEY_REQUIRED, QJsonArray::fromStringList(tool.required));

    QJsonObject propertiesObj;
    for (const ModelToolProperty &prop : tool.properties) {
        QJsonObject propObj;
        propObj.insert(STR_KEY_TYPE, prop.type);
        propObj.insert(STR_KEY_DESCRIPTION, prop.description);
        if (!prop.enums.isEmpty())
            propObj.insert(STR_KEY_ENUM, QJsonArray::fromStringList(prop.enums));
        propertiesObj.insert(prop.name, propObj);
    }

    parametersObj.insert(STR_KEY_PROPERTIES, propertiesObj);
    toolObj.insert(STR_KEY_PARAMETERS, parametersObj);
    m_tools.append(toolObj);
}

void OaiMessageProtocol::addTools(const QList<ModelTool> &tools)
{
    for (const ModelTool &tmp : tools)
        addTool(tmp);
}

QJsonObject OaiMessageProtocol::params(const QVariantHash &args)
{
    QJsonObject ret;
    if (args.contains(STR_KEY_MAX_TOKENS))
        ret.insert(STR_KEY_MAX_TOKENS, args.value(STR_KEY_MAX_TOKENS).toInt());

    if (args.contains(STR_KEY_TEMPERATURE))
        ret.insert(STR_KEY_TEMPERATURE, args.value(STR_KEY_TEMPERATURE).toDouble());

    if (args.contains(STR_KEY_STREAM)) {
        ret.insert(STR_KEY_STREAM, args.value(STR_KEY_STREAM).toBool());
        ret.insert("stream_options", QJsonObject{{"include_usage", true}});
    }

    if (args.contains(STR_KEY_THINKING)) {
        QJsonObject thinkingObj;
        thinkingObj.insert(STR_KEY_TYPE, args.value(STR_KEY_THINKING).toBool() ? "enabled" : "disabled");
        ret.insert(STR_KEY_THINKING, thinkingObj);
    }

    if (args.contains(STR_KEY_TOOLS)) {
        m_tools = QJsonArray();
        m_toolNameMap.clear();
        ModelToolList toolList = args.value(STR_KEY_TOOLS).value<ModelToolList>();
        if (!toolList.isEmpty()) {
            addTools(toolList);
            ret[STR_KEY_TOOLS] = this->tools();
        }
    }

    return ret;
}

QJsonArray OaiMessageProtocol::messages() const
{
    if (!m_reasoning) {
        QJsonArray filteredMsg;
        for (QJsonValue msg : m_messages) {
            auto obj = msg.toObject();
            obj.remove(STR_KEY_REASONING_CONTENT);
            filteredMsg.append(obj);
        }
        return filteredMsg;
    }

    return m_messages;
}

QJsonArray OaiMessageProtocol::tools() const
{
    QJsonArray ret;
    for (const QJsonValue &func : m_tools) {
        QJsonObject funcObj;
        funcObj.insert(STR_KEY_TYPE, STR_KEY_FUNCTION);
        funcObj.insert(STR_KEY_FUNCTION, func);
        ret.append(funcObj);
    }

    return ret;
}

bool OaiMessageProtocol::parseChunk(const QByteArray &chunkData, QVariantHash &content)
{
    // 将新数据添加到缓冲区
    {
        auto tmp = m_buffer.toUtf8();
        tmp.append(chunkData);
        m_buffer = QString::fromUtf8(tmp);
    }
    
    // 使用正则表达式匹配完整的数据块
    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator it = regex.globalMatch(m_buffer);

    // 记录处理过的数据位置
    int lastProcessedPos = 0;
    QString result;
    QString reasoningContent;
    QMap<int, QJsonObject> toolCallMaps;
    QVariantList usage;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        QString matchString = match.captured(0);
        int startIndex = matchString.indexOf('{');
        int endIndex = matchString.lastIndexOf('}');

        // 记录当前匹配的位置
        lastProcessedPos = match.capturedEnd(0);
        if (startIndex < 0 || endIndex <= startIndex)
            continue;

        {
            QString matchContent = matchString.mid(startIndex, endIndex - startIndex + 1);
            QJsonObject rootObject = QJsonDocument::fromJson(matchContent.toUtf8()).object();
            if (usage.isEmpty() && rootObject.contains(STR_KEY_USAGE)) {
                auto hash = rootObject[STR_KEY_USAGE].toObject().toVariantHash();
                if (!hash.isEmpty()) {
                    usage.append(hash.value("prompt_tokens").toInt());
                    usage.append(hash.value("completion_tokens").toInt());
                    usage.append(hash.value("total_tokens").toInt());
                }
            }
            
            if (!rootObject.contains(STR_KEY_CHOICES))
                continue;
            QJsonArray choicesArray = rootObject[STR_KEY_CHOICES].toArray();
            if (choicesArray.isEmpty())
                continue;

            QJsonObject choiceObject = choicesArray.at(0).toObject();
            if (!choiceObject.contains(STR_KEY_DELTA))
                continue;
            QJsonObject deltaObject = choiceObject[STR_KEY_DELTA].toObject();

            if (deltaObject.contains(STR_KEY_CONTENT)) {
                result += deltaObject.value(STR_KEY_CONTENT).toString();
            }
            if (deltaObject.contains(STR_KEY_REASONING_CONTENT)) {
                reasoningContent += deltaObject.value(STR_KEY_REASONING_CONTENT).toString();
            }

            if (deltaObject.contains(STR_KEY_TOOL_CALLS)) {
                const QJsonArray &tool_calls = deltaObject.value(STR_KEY_TOOL_CALLS).toArray();
                for (const QJsonValue &tool_call : tool_calls) {
                    const QJsonObject &toolCallObj = tool_call.toObject();

                    int index = toolCallObj[STR_KEY_INDEX].toInt();
                    if (!toolCallMaps[index].contains(STR_KEY_FUNCTION)) {
                        toolCallMaps[index][STR_KEY_FUNCTION] = QJsonObject();
                    }

                    toolCallMaps[index][STR_KEY_INDEX] = index;

                    if (toolCallObj.contains(STR_KEY_ID)) {
                        toolCallMaps[index][STR_KEY_ID] = toolCallObj.value(STR_KEY_ID);
                    }

                    if (toolCallObj.contains(STR_KEY_TYPE)) {
                        toolCallMaps[index][STR_KEY_TYPE] = toolCallObj.value(STR_KEY_TYPE);
                    }

                    if (toolCallObj.contains(STR_KEY_FUNCTION)) {
                        QJsonObject toolFun = toolCallMaps[index][STR_KEY_FUNCTION].toObject();
                        const QJsonObject &tmpToolFun =  toolCallObj.value(STR_KEY_FUNCTION).toObject();
                        if (tmpToolFun.contains(STR_KEY_NAME)) {
                            toolFun[STR_KEY_NAME] = toolFun[STR_KEY_NAME].toString() + tmpToolFun.value(STR_KEY_NAME).toString();
                        }
                        if (tmpToolFun.contains(STR_KEY_ARGUMENTS)) {
                            toolFun[STR_KEY_ARGUMENTS] = toolFun[STR_KEY_ARGUMENTS].toString() + tmpToolFun.value(STR_KEY_ARGUMENTS).toString();
                        }

                        toolCallMaps[index][STR_KEY_FUNCTION] = toolFun;
                    }
                }
            }
        }
    }
    
    // 移除已处理的数据，保留未处理的部分
    if (lastProcessedPos > 0) {
        m_buffer = m_buffer.mid(lastProcessedPos);
    }

    if (result.isEmpty() && reasoningContent.isEmpty() && toolCallMaps.isEmpty())
        return false;

    if (!result.isEmpty())
        content[STR_KEY_CONTENT] = result;

    if (!reasoningContent.isEmpty())
        content[STR_KEY_REASONING_CONTENT] = reasoningContent;

    ModelToolCallList toolCalls;
    {
        QList<int> keys = toolCallMaps.keys();
        std::stable_sort(keys.begin(), keys.end());
        for (int index : keys) {
            ModelToolCall toolCall;
            auto obj = toolCallMaps[index];
            toolCall.id = obj[STR_KEY_ID].toString();
            auto func = obj[STR_KEY_FUNCTION].toObject();
            toolCall.name = restoreToolName(func.value(STR_KEY_NAME).toString());
            toolCall.arguments = QJsonDocument::fromJson(func.value(STR_KEY_ARGUMENTS).toString().toUtf8()).object().toVariantHash();

            if (toolCall.isValid()) {
                toolCalls.append(toolCall);
            }
        }
    }

    if (!toolCalls.isEmpty())
        content[STR_KEY_TOOL_CALLS] = QVariant::fromValue(toolCalls);

    if (!usage.isEmpty())
        content[STR_KEY_USAGE] = usage;

    return true;
}

bool OaiMessageProtocol::parseResponse(const QByteArray &data, QVariantHash &content)
{
    QJsonObject rootObject = QJsonDocument::fromJson(data).object();
    if (!rootObject.contains(STR_KEY_CHOICES))
        return false;

    QJsonArray choicesArray = rootObject[STR_KEY_CHOICES].toArray();
    if (choicesArray.isEmpty())
        return false;

    QJsonObject messageObject = choicesArray.at(0).toObject()[STR_KEY_MESSAGE].toObject();

    QString result = messageObject.value(STR_KEY_CONTENT).toString();
    QString reasoningContent = messageObject.value(STR_KEY_REASONING_CONTENT).toString();

    if (!result.isEmpty())
        content[STR_KEY_CONTENT] = result;

    if (!reasoningContent.isEmpty())
        content[STR_KEY_REASONING_CONTENT] = reasoningContent;

    if (messageObject.contains(STR_KEY_TOOL_CALLS)) {
        ModelToolCallList toolCalls;
        for (const QJsonValue &val : messageObject[STR_KEY_TOOL_CALLS].toArray()) {
            QJsonObject toolCallObj = val.toObject();
            ModelToolCall toolCall;
            toolCall.id = toolCallObj[STR_KEY_ID].toString();
            QJsonObject func = toolCallObj[STR_KEY_FUNCTION].toObject();
            toolCall.name = restoreToolName(func.value(STR_KEY_NAME).toString());
            toolCall.arguments = QJsonDocument::fromJson(func.value(STR_KEY_ARGUMENTS).toString().toUtf8()).object().toVariantHash();
            if (toolCall.isValid())
                toolCalls.append(toolCall);
        }
        if (!toolCalls.isEmpty())
            content[STR_KEY_TOOL_CALLS] = QVariant::fromValue(toolCalls);
    }

    if (rootObject.contains(STR_KEY_USAGE)) {
        auto hash = rootObject[STR_KEY_USAGE].toObject().toVariantHash();
        if (!hash.isEmpty()) {
            QVariantList usage;
            usage.append(hash.value("prompt_tokens").toInt());
            usage.append(hash.value("completion_tokens").toInt());
            usage.append(hash.value("total_tokens").toInt());
            content[STR_KEY_USAGE] = usage;
        }
    }

    return !result.isEmpty() || !reasoningContent.isEmpty() || content.contains(STR_KEY_TOOL_CALLS);
}

void OaiMessageProtocol::clearBuffer()
{
    m_buffer.clear();
}

void OaiMessageProtocol::setEnableReasoning(bool enable)
{
    m_reasoning = enable;
}

QJsonObject OaiMessageProtocol::buildMessage(const ModelMessage &msg)
{
    if (msg.content.isEmpty())
        return QJsonObject();

    QJsonObject msgObj;
    msgObj[STR_KEY_ROLE] = msg.role;

    if (msg.role.compare(STR_KEY_TOOL) == 0) {
        if (msg.content.size() != 1) {
            qCWarning(logModel) << "Tool message must have only one content";
        }

        for (const MetaMessage &cnt : msg.content) {
            auto hash = cnt.data.toHash();
            auto id = hash.value(STR_KEY_TOOL_CALL_ID).toString();
            if (id.isEmpty()) 
                continue;
            
            msgObj[STR_KEY_TOOL_CALL_ID] = id;
            msgObj[STR_KEY_CONTENT] = hash.value(STR_KEY_CONTENT).toString();
            return msgObj;
        } 

        qCWarning(logModel) << "Invalid tool message, must have tool call id";
        return QJsonObject();
    } else {
        QJsonArray toolCallsArray;
        QJsonArray contentArray;

        bool hasImage = std::any_of(msg.content.begin(), msg.content.end(), [](const MetaMessage &cnt) {
            return cnt.type == ContentType::CntImage;
        });

        for (const MetaMessage &cnt : msg.content) {
            if (cnt.type == ContentType::CntTool) {
                ModelToolCallList calls = cnt.data.value<ModelToolCallList>();
                for (const ModelToolCall &call : calls) {
                    toolCallsArray.append(buildToolCall(call));
                }
            } else if (cnt.type == ContentType::CntReasoning) {
                msgObj[STR_KEY_REASONING_CONTENT] = cnt.data.toString();
            } else if (cnt.type == ContentType::CntText) {
                if (hasImage)
                    contentArray.append(buildContent(cnt));
                else
                    msgObj[STR_KEY_CONTENT] = cnt.data.toString();
            } else if (cnt.type == ContentType::CntImage) {
                contentArray.append(buildContent(cnt));
            }
        }

        if (!toolCallsArray.isEmpty())
            msgObj[STR_KEY_TOOL_CALLS] = toolCallsArray;

        if (!contentArray.isEmpty())
            msgObj[STR_KEY_CONTENT] = contentArray;

        if (!msgObj.contains(STR_KEY_CONTENT) && !msgObj.contains(STR_KEY_TOOL_CALLS))
            msgObj[STR_KEY_CONTENT] = QString();
    }

    return msgObj;
}

QJsonObject OaiMessageProtocol::buildContent(const MetaMessage &msg)
{
    QJsonObject contentObj;
    QString strType = convertContentType(msg.type);
    if (strType.isEmpty())
        return contentObj;

    contentObj[STR_KEY_TYPE] = strType;
    if (msg.type == ContentType::CntText) {
        contentObj[strType] = msg.data.toString();
    } else if (msg.type == ContentType::CntImage) {
        QString imageData = QString("data:image/png;base64,%0").arg(msg.data.toString());
        contentObj[strType] = QJsonObject{{"url", imageData}};
    }

    return contentObj;
}

QJsonObject OaiMessageProtocol::buildToolCall(const ModelToolCall &toolCall)
{
    QJsonObject callObj;

    callObj[STR_KEY_ID] = toolCall.id;
    callObj[STR_KEY_TYPE] = STR_KEY_FUNCTION;

    QJsonObject functionObj;
    functionObj[STR_KEY_NAME] = sanitizeToolName(toolCall.name);
    {
        QJsonObject argumentsObj = QJsonObject::fromVariantHash(toolCall.arguments);
        functionObj[STR_KEY_ARGUMENTS] = QString::fromUtf8(QJsonDocument(argumentsObj).toJson(QJsonDocument::Compact));
    }

    callObj[STR_KEY_FUNCTION] = functionObj;
    return callObj;
}

QString OaiMessageProtocol::sanitizeToolName(const QString &name)
{
    static const QRegularExpression validName(QStringLiteral("^[a-zA-Z0-9_-]+$"));
    if (validName.match(name).hasMatch())
        return name;

    QString sanitized = name;
    sanitized.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    if (sanitized.isEmpty())
        sanitized = QStringLiteral("tool");

    if (!sanitized.at(0).isLetterOrNumber())
        sanitized.prepend(QStringLiteral("tool_"));

    m_toolNameMap.insert(sanitized, name);

    return sanitized;
}

QString OaiMessageProtocol::restoreToolName(const QString &name) const
{
    return m_toolNameMap.value(name, name);
}

QString OaiMessageProtocol::convertContentType(ContentType contentType)
{
    switch (contentType) {
    case ContentType::CntText:
        return "text";
    case ContentType::CntImage:
        return "image_url";
    default:
        return "";
    }
}
