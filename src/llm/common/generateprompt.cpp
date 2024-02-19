#include "generateprompt.h"

#include <QDebug>

QString GeneratePrompt::cmdSystemRole()
{
    return "You are a very powerful command formatting system.";
}

QString GeneratePrompt::cmdPrompt(const QMap<QString, QVariantMap> &appCmdPrompts, const QString &textInput)
{
    QStringList availableCommands;

    for (auto appIdIter = appCmdPrompts.begin(); appIdIter != appCmdPrompts.end(); ++appIdIter) {
        if (appIdIter.value().isEmpty())
            continue;

        QStringList cmds;
        const QVariantMap &cmdPrompts = appIdIter.value();

        for (auto cmdIter = cmdPrompts.begin(); cmdIter != cmdPrompts.end(); ++cmdIter) {
            const QString &command = cmdIter.key();
            const QString &description = cmdIter.value().toString();

            if (description.isEmpty() || command.isEmpty())
                continue;

            cmds << QString("%1(%2)").arg(command).arg(description);
        }

        if (!cmds.isEmpty())
            availableCommands << QString("Commands for %1: %2").arg(appIdIter.key()).arg(cmds.join(", "));
    }

    if (availableCommands.isEmpty()) {
        qInfo() << "No available commands found!";
        return QString();
    }

    QString output = availableCommands.join(". ") + ".\n";
    output += "Format the input content as: {{applicationName, command, parameters}}. "
              "Each set of curly braces should contain only one instruction, and multiple instructions should be separated by commas.\n";
    output += "The output language should be the same as the input language.\n";
    output += "Input: " + textInput + "\n";
    output += "Output:";

    return output;
}

QString GeneratePrompt::Translate2EnPrompt(const QString &userData)
{
    QString output;
    output += "请你扮演一个英文翻译，你可以将任何语言翻译为英文。我会说一段话，你需要帮我把它翻译为英文。请你不要重复我说的话。请你不要将我说的话翻译为除英文外的其它语言。请只回复翻译内容，不要在回复内容添加任何注释、说明或者其它内容。请你从下面的内容开始：\n";
    output += userData;
    return output;
}
