#ifndef PARSERSPPROMPT_H
#define PARSERSPPROMPT_H

#include <QVariantMap>

class ParseRspPrompt
{
public:
    /**
     * @brief parseChatCmdPromptResponse
     * @param response
     * @param appCmdPrompts
     * @return
     */
    static QMap<QString, QVariantMap> chatCmdPromptResponse(const QString &response, const QMap<QString, QVariantMap> &appCmdPrompts);

private:
    /**
     * @brief perfectString
     * @param data
     * @return
     */
    static QString perfectString(QString data);
};

#endif // PARSERSPPROMPT_H
