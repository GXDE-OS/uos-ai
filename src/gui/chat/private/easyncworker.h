#ifndef EASYNCWORKER_H
#define EASYNCWORKER_H

#include "eaiproxy.h"
#include "esinglewebview.h"
#include "esingleton.h"

#include <QObject>

namespace uos_ai {
class EAsyncWorker : public QObject
{
    Q_OBJECT

    SINGLETONIZE_CLASS(EAsyncWorker)

    explicit EAsyncWorker(QObject *parent = nullptr);
public:
    enum AsyncWorkerType {
        None = 0,
        OverrideInput,
        AppendPrompt,
        ChangeToUosAI,
        AppendImage,
    };
    // TODO: 任务队列
    bool executeSynchronouslyGeneric(AsyncWorkerType type);
    bool chatInitSyncWorker();

signals:
    void sigAsyncWorker(int type);
    void sigWorkerFinished(int type);
    void sigChatInitSyncWorkerFinished();
};
}
#define EAsync() (ESingleton<EAsyncWorker>::getInstance())
#endif // EASYNCWORKER_H
