#ifndef WRITINGORCHESTRATOR_H
#define WRITINGORCHESTRATOR_H

#include "llmagent.h"
#include "writingworkspace.h"

#include <QSharedPointer>

namespace uos_ai {

class WritingMode;
class WritingStage;

/**
 * WritingOrchestrator: AI 写作流程的统一编排器
 *
 * 职责：
 * 1. 意图分类（LLM-based）
 * 2. 通过 WritingMode 创建阶段序列
 * 3. 顺序执行各 WritingStage
 *
 * 替代旧的 WritingMasterAgent + WritingPipeline 组合。
 */
class WritingOrchestrator : public LlmAgent
{
    Q_OBJECT
public:
    explicit WritingOrchestrator(QObject *parent = nullptr);
    ~WritingOrchestrator() override;

    bool initialize() override;

    QVariantHash processRequest(const ModelMessage &question,
                               const QList<ModelMessage> &history,
                               const QVariantHash &params = {}) override;

    void cancel() override;

    void setWorkspace(WritingWorkspace *workspace);

private:
    QString classifyIntent(const ModelMessage &msg);

    /** 解析上传素材文件，返回可直接存入 workspace.references 的引用数组 */
    static QJsonArray parseFilesToReferences(const QStringList &files);

    WritingMode *m_mode = nullptr;
    WritingWorkspace *m_workspace = nullptr;
    WritingStage *m_activeStage = nullptr;
};

} // namespace uos_ai

#endif // WRITINGORCHESTRATOR_H
