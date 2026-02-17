#include "toolparser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

ToolParser::ToolParser(QObject *parent) : QObject(parent)
{
}

void ToolParser::feed(const QString &input)
{
    for (int i = 0; i < input.size(); ++i) {
        QString chunk = input.at(i);
        if (!m_inFunction) {
            // 检查是否可能开始function标签
            if (!m_pendingTag.isEmpty()) {
                m_pendingTag += chunk;
                checkPendingTag();
            } else if (chunk.contains('<')) {
                int idx = chunk.indexOf('<');
                pushResult(Text, chunk.left(idx));
                m_pendingTag = "<" + chunk.mid(idx + 1);
                checkPendingTag();
            } else {
                pushResult(Text, chunk);
            }
        } else {
            m_buffer += chunk;
            // 检查是否完成function标签
            if (m_buffer.contains("</function>")) {
                processFunction();
            }
        }
    }
}

void ToolParser::checkPendingTag()
{
    // 检查pending_tag是否是function标签的开始
    static const QString tagName = "<function>";
    if (m_pendingTag.startsWith(tagName)) {
        // 可能是function标签开始
        if (m_pendingTag.length() >= tagName.length()) {  // "<function>".length
            m_inFunction = true;
            m_buffer = m_pendingTag;
            m_pendingTag.clear();
        }
        // 否则等待更多字符确认
    } else if (!tagName.startsWith(m_pendingTag)) {
        // 确定不是function标签
        pushResult(Text, m_pendingTag);
        m_pendingTag.clear();
    }
}

void ToolParser::processFunction()
{
    QRegularExpression regex("<function>(.*?)</function>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = regex.match(m_buffer);
    
    if (match.hasMatch()) {
        pushResult(Function, match.captured(1));

        // 处理</function>后面的内容
        QString remainingContent = m_buffer.mid(match.capturedEnd());
        if (!remainingContent.isEmpty()) {
            qCCritical(logAgent) << "After parsing, there will be remainin content:" << remainingContent;
#ifdef QT_DEBUG
            abort();
#endif
            // 将剩余内容作为新输入处理
//            m_buffer.clear();
//            m_inFunction = false;
//            m_pendingTag.clear();
//            m_chunk = remainingContent;
            return;
        }
    }
    m_buffer.clear();
    m_inFunction = false;
    m_pendingTag.clear();
}

void ToolParser::finalize()
{
    // 结束输入，处理剩余buffer内容
    if (m_inFunction || !m_pendingTag.isEmpty()) {
        // 将剩余内容作为普通文本处理
        pushResult(Text, m_pendingTag + m_buffer);

        m_pendingTag.clear();
        m_buffer.clear();
        m_inFunction = false;
    }
}

QList<QPair<int, QString> > ToolParser::getResults()
{
    auto ret = m_results;
    m_results.clear();
    return ret;
}

bool ToolParser::toJson(const QString &func_content, QString &functionName, QJsonObject &toolArgs)
{
    functionName.clear();
    toolArgs = QJsonObject();

    QJsonDocument funcDoc;
    // 尝试解析为完整JSON
    funcDoc = QJsonDocument::fromJson(func_content.toUtf8());
    if (!funcDoc.isNull() && funcDoc.isObject()) {
        QJsonObject funcObj = funcDoc.object();
        functionName = funcObj["name"].toString().trimmed();
        toolArgs = funcObj["parameters"].toObject();
    }
    // 如果JSON解析失败，尝试回退处理
    else {
        QStringList lines = func_content.trimmed().split('\n');
        if (!lines.isEmpty()) {
            functionName = lines.first().trimmed();
            QString jsonData = lines.mid(1).join("");
            QJsonDocument fallbackDoc = QJsonDocument::fromJson(jsonData.toUtf8());
            if (!fallbackDoc.isNull() && fallbackDoc.isObject()) {
                toolArgs = fallbackDoc.object();
            }
        }
    }

    return !functionName.isEmpty();
}

QString ToolParser::restoreFunction(const QString &content)
{
    return QString("<function>%0</function>\n\n").arg(content);
}

void ToolParser::pushResult(ToolParser::Type type, const QString &content)
{
    if (type == Text) {
        if (m_results.isEmpty()) {
            m_results.append(qMakePair(Text, content));
            return;
        }

        auto &ref = m_results.last();
        if (ref.first == Text)
            ref.second.append(content);
        else
            m_results.append(qMakePair(Text, content));
    } else if (type == Function) {
        m_results.append(qMakePair(Function, content));
    }
}
