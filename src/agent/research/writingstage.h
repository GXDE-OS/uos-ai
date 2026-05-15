#ifndef WRITINGSTAGE_H
#define WRITINGSTAGE_H

#include "llmagent.h"
#include "model/abstractchatmodel.h"

#include <QObject>
#include <QSharedPointer>

namespace uos_ai {

class WritingWorkspace;

/**
 * WritingStage: 写作管线阶段抽象基类
 *
 * 每个阶段自行封装：
 * 1. 从 WritingWorkspace 读取输入
 * 2. 创建并运行子 Agent
 * 3. 将产物写回 WritingWorkspace
 */
class WritingStage : public QObject
{
    Q_OBJECT
public:
    explicit WritingStage(QObject *parent = nullptr);
    virtual ~WritingStage() = default;

    virtual QString id() const = 0;

    /**
     * 执行阶段
     * @param ws      写作工作区（读输入、写输出）
     * @param userMsg 当前用户消息
     * @param history 历史消息
     * @param params  扩展参数
     */
    virtual QVariantHash execute(WritingWorkspace &ws,
                                  const ModelMessage &userMsg,
                                  const QList<ModelMessage> &history,
                                  const QVariantHash &params) = 0;

    /** 交互式阶段（如 outline）执行后暂停管线 */
    virtual bool isInteractive() const { return false; }

    /** 该阶段的结果是否纳入历史 context（默认 true） */
    virtual bool contributeToContext() const { return true; }

    void setModel(QSharedPointer<AbstractChatModel> llm);
    void setModelParams(const QVariantHash &params);
    void cancel();

    QVariantHash lastError() const { return m_lastError; }

Q_SIGNALS:
    void messageReceived(const RenderMessageList &msgs);

protected:
    QSharedPointer<AbstractChatModel> m_llm;
    QVariantHash m_modelParams;
    LlmAgent *m_activeAgent = nullptr;
    bool m_canceled = false;
    QVariantHash m_lastError;
};

// --- 具体阶段 ---

class OutlineStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("outline"); }
    bool isInteractive() const override { return true; }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class ResearchStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("research"); }
    bool contributeToContext() const override { return false; }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class ComposeStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("write"); }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class PredictStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("predict"); }
    bool contributeToContext() const override { return false; }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class AdjustStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("adjust"); }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class ChatStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("chat"); }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class RollbackStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("rollback"); }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

class NewArticleStage : public WritingStage
{
public:
    using WritingStage::WritingStage;
    QString id() const override { return QStringLiteral("new_article"); }
    QVariantHash execute(WritingWorkspace &ws, const ModelMessage &userMsg,
                          const QList<ModelMessage> &history, const QVariantHash &params) override;
};

} // namespace uos_ai

#endif // WRITINGSTAGE_H
