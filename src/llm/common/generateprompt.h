#ifndef GENERATEPROMPT_H
#define GENERATEPROMPT_H

#include <QVariantMap>

class GeneratePrompt
{
public:
    /**
     * @brief cmdSystemRole
     * @return
     */
    static QString cmdSystemRole();

    /**
     * @brief cmdPrompt
     * @param appCmdPrompts
     * @param prompt
     * @return
     */
    static QString cmdPrompt(const QMap<QString, QVariantMap> &appCmdPrompts, const QString &textInput);

    /**
     * @brief Translate2EnPrompt
     * @return
     */
    static QString Translate2EnPrompt(const QString &userData);
};

#endif // GENERATEPROMPT_H
