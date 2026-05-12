#include "minimaxmessageprotocol.h"
#include "global_key_define.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpressionMatchIterator>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

MiniMaxMessageProtocol::MiniMaxMessageProtocol() : OaiMessageProtocol()
{
}

MiniMaxMessageProtocol::~MiniMaxMessageProtocol()
{
}

QJsonObject MiniMaxMessageProtocol::params(const QVariantHash &args)
{
    QJsonObject ret = OaiMessageProtocol::params(args);

    if (args.contains(STR_KEY_THINKING)) {
        bool thinkingEnabled = args.value(STR_KEY_THINKING).toBool();
        ret.remove(STR_KEY_THINKING);
        ret.insert("reasoning_split", thinkingEnabled);
    }

    return ret;
}

bool MiniMaxMessageProtocol::parseChunk(const QByteArray &chunkData, QVariantHash &content)
{
    {
        auto tmp = m_buffer.toUtf8();
        tmp.append(chunkData);
        m_buffer = QString::fromUtf8(tmp);
    }
    
    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator it = regex.globalMatch(m_buffer);

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
            
            if (deltaObject.contains("reasoning_details")) {
                QJsonArray reasoningDetails = deltaObject.value("reasoning_details").toArray();
                for (const QJsonValue &detail : reasoningDetails) {
                    QJsonObject detailObj = detail.toObject();
                    if (detailObj.contains("text")) {
                        reasoningContent += detailObj.value("text").toString();
                    }
                }
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
            toolCall.name = func.value(STR_KEY_NAME).toString();
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

bool MiniMaxMessageProtocol::parseResponse(const QByteArray &data, QVariantHash &content)
{
    QJsonObject rootObject = QJsonDocument::fromJson(data).object();
    if (!rootObject.contains(STR_KEY_CHOICES))
        return false;

    QJsonArray choicesArray = rootObject[STR_KEY_CHOICES].toArray();
    if (choicesArray.isEmpty())
        return false;

    QJsonObject messageObject = choicesArray.at(0).toObject()[STR_KEY_MESSAGE].toObject();

    QString result = messageObject.value(STR_KEY_CONTENT).toString();
    QString reasoningContent;
    
    if (messageObject.contains("reasoning_details")) {
        QJsonArray reasoningDetails = messageObject.value("reasoning_details").toArray();
        for (const QJsonValue &detail : reasoningDetails) {
            QJsonObject detailObj = detail.toObject();
            if (detailObj.contains("text")) {
                reasoningContent += detailObj.value("text").toString();
            }
        }
    }

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
            toolCall.name = func.value(STR_KEY_NAME).toString();
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
