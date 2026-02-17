#ifndef INSTRUCTIONMANAGER_H
#define INSTRUCTIONMANAGER_H

#include "serverdefs.h"

#include <QObject>

class EAiPrompt;
class EAiCallback;
class EAiInstructionPrompt;
namespace uos_ai {
class InstructionWorker;
class InstructionManager : public QObject
{
    Q_OBJECT

public:
    enum Instructions {
        None          = -1,
        SystemControl = 0,  // 系统控制
        LaunchApp,          // 启动应用
        SendMail,
        CreateSchedule,
        GenerateImage,
        SearchOnline,
        MultimediaControl,  // 多媒体控制
    };

    static InstructionManager *instance();

    QString query(AssistantType type, LLMChatModel model);

    QSharedPointer<EAiPrompt> genPrompt(int inst, const QString &userData);

public slots:
    QString onAiCallback(int inst, QString aiReply);

private:
    InstructionManager();
    void init();

    QMap<int, QSharedPointer<InstructionWorker>> instWorkers;
};
}
#endif // INSTRUCTIONMANAGER_H
