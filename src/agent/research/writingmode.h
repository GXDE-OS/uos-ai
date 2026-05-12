#ifndef WRITINGMODE_H
#define WRITINGMODE_H

#include <QObject>
#include <QString>
#include <QList>

namespace uos_ai {

class WritingStage;
class WritingWorkspace;
struct ModelMessage;

/**
 * WritingMode: 写作模式的抽象接口
 *
 * 每种写作模式根据当前 Workspace 状态和用户意图，
 * 创建对应的 WritingStage 执行序列。
 * 新增模式只需实现此接口并在 WritingOrchestrator 中注册。
 */
class WritingMode
{
public:
    virtual ~WritingMode() = default;

    virtual QString name() const = 0;

    /**
     * 根据 Workspace 状态和用户意图，创建本次执行的阶段序列。
     *
     * @param ws         当前 Workspace 数据状态
     * @param userMsg    用户消息
     * @param userIntent LLM 意图分类结果（"outline"/"research"/"adjust"/"chat"，空串为未分类）
     * @param parent     阶段对象的 QObject 父对象
     * @return 有序的 WritingStage 列表，调用者负责执行并删除
     */
    virtual QList<WritingStage*> createStages(const WritingWorkspace &ws,
                                               const ModelMessage &userMsg,
                                               const QString &userIntent,
                                               QObject *parent) const = 0;
};

} // namespace uos_ai

#endif // WRITINGMODE_H
