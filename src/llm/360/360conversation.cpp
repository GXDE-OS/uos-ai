#include "360conversation.h"

#include <QJsonDocument>
#include <QRegularExpression>

Conversation360::Conversation360()
{

}

QString Conversation360::parseContentString(const QString &content)
{
    QString deltacontent;

    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator iter = regex.globalMatch(content);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matchString = match.captured(0);

        int startIndex = matchString.indexOf('{');
        int endIndex = matchString.lastIndexOf('}');

        if (startIndex >= 0 && endIndex > startIndex) {
            QString content = matchString.mid(startIndex, endIndex - startIndex + 1);

            QJsonObject j = QJsonDocument::fromJson(content.toUtf8()).object();
            if (j.contains("choices")) {
                const QJsonArray &choices = j["choices"].toArray();
                for (auto choice = choices.begin(); choice != choices.end(); choice++) {
                    const QJsonObject &cj = choice->toObject();
                    if (cj.contains("delta")) {
                        const QString &deltaData = cj["delta"]["content"].toString();
                        deltacontent += deltaData;
                    }
                }
            }
        }
    }

    return deltacontent;
}

void Conversation360::update(const QByteArray &response)
{
    if (response.isEmpty())
        return;

    if (response.startsWith("data:")) {
        const QString &delateData = parseContentString(response);
        if (!delateData.isEmpty()) {
            m_conversion.push_back(QJsonObject({
                { "role",    "assistant"    },
                { "content", delateData }
            }));
        }
    } else {
        const QJsonObject &j = QJsonDocument::fromJson(response).object();
        if (j.contains("choices")) {
            const QJsonArray &choices = j["choices"].toArray();
            for (auto choice = choices.begin(); choice != choices.end(); choice++) {
                const QJsonObject &cj = choice->toObject();
                if (cj.contains("message")) {
                    if (!cj["message"]["role"].isNull() && !cj["message"]["content"].isNull()) {
                        m_conversion.push_back(QJsonObject({
                            { "role",    cj["message"]["role"]    },
                            { "content", cj["message"]["content"] }
                        }));
                    }
                }
            }
        } else if (j.contains("message")) {
            if (!j["message"]["role"].isNull() && !j["message"]["content"].isNull()) {
                m_conversion.push_back(QJsonObject({
                    { "role",    j["message"]["role"]    },
                    { "content", j["message"]["content"] }
                }));
            }
        } else if (j.contains("role") && j.contains("content")) {
            m_conversion.push_back(QJsonObject({
                { "role",    j["role"]    },
                { "content", j["content"] }
            }));
        }
    }
}
