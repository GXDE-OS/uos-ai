#include "zhipuconversation.h"
#include "zhipucodetranslation.h"
#include "networkdefs.h"

#include <QJsonDocument>
#include <QRegularExpression>

ZhiPuConversation::ZhiPuConversation()
{

}

QPair<int, QString> ZhiPuConversation::parseContentString(const QString &content)
{
    QString deltacontent;
    QRegularExpression regex("((event:(?<event>[^\\n]+))|(id:(?<id>[^\\n]+))|(data:(?<data>[^\\n]+)))");
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(content);

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        QString event = match.captured("event");
        QString id = match.captured("id");
        QString data = match.captured("data");

        if (content.contains("event:error")) {
            QRegExp regex("\\[([^\\[\\]]*)\\]\\[([^\\[\\]]*)\\]"); // 正则表达式用于匹配连续的两个中括号及其内容

            int pos = 0;
            while ((pos = regex.indexIn(data, pos)) != -1) {
                QString matchedText = regex.cap(0);

                bool ok = false;
                int code = regex.cap(1).toInt(&ok);
                QString errorMessage = regex.cap(2);
                pos += matchedText.length();

                if (ok && code > 0) {
                    AIServer::ErrorType errorType = AIServer::NoError;
                    if (code == 1302 || code == 1303 || code == 1305) {
                        errorType = AIServer::ServerRateLimitError;
                    } else {
                        errorType = AIServer::ContentAccessDenied;
                    }
                    const QString &message = ZhiPuCodeTranslation::serverCodeTranlation(code, errorMessage);
                    return qMakePair(errorType, message);
                }
            }
        }

        if (!data.isEmpty()) {
            deltacontent += data;
        }
    }

    return qMakePair(AIServer::NoError, deltacontent);
}

QPair<int, QString> ZhiPuConversation::update(const QByteArray &response)
{
    if (response.isEmpty())
        return QPair<int, QString>(0, QString());

    const QPair<int, QString> &resultPair = parseContentString(response);
    if (resultPair.first == 0 && !resultPair.second.isEmpty()) {
        m_conversion.push_back(QJsonObject({
            { "role",    "assistant"    },
            { "content", resultPair.second }
        }));

        return QPair<int, QString>(0, QString());
    }

    return resultPair;
}
