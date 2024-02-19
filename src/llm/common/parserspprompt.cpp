#include "parserspprompt.h"

#include <QRegularExpression>
#include <QDebug>

QString ParseRspPrompt::perfectString(QString data)
{
    return data.remove(QRegExp("[\"]"));
}

QMap<QString, QVariantMap> ParseRspPrompt::chatCmdPromptResponse(const QString &response, const QMap<QString, QVariantMap> &srcAppCmdPrompts)
{
    QRegularExpression regex("\\{([^{}]*,[^{}]*,[^{}]*)\\}");
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(response);

    QMap<QString, QVariantMap> appCmdPrompts;
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        QString matchText = match.captured(1).replace("，", ",");
        QStringList items = matchText.split(',');

        QString appId = perfectString(items.value(0).trimmed());
        QString cmd = perfectString(items.value(1).trimmed());

        if (items.size() >= 3
                && srcAppCmdPrompts.contains(appId)
                && srcAppCmdPrompts[appId].contains(cmd)) {
            items.removeFirst();
            items.removeFirst();
            appCmdPrompts[appId][cmd] = items.join(",").trimmed();
        }
    }

    if (appCmdPrompts.isEmpty()) {
        qWarning() << "ResponseParser::parseChatCmdPromptResponse error = " << response;
    }

    return appCmdPrompts;
}
