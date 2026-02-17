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
            QRegularExpression regex("\\[([^\\[\\]]*)\\]\\[([^\\[\\]]*)\\]");
            QRegularExpressionMatchIterator matchIterator = regex.globalMatch(data);

           while (matchIterator.hasNext()) {
                QRegularExpressionMatch subMatch = matchIterator.next();
                bool ok = false;
                int code = subMatch.captured(1).toInt(&ok);
                QString errorMessage = subMatch.captured(2);
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
