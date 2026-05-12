#ifndef ASSISTANTCHANNEL_H
#define ASSISTANTCHANNEL_H

#include <QObject>
#include <QVariantMap>
#include <QJsonObject>

namespace uos_ai {

class AssistantChannel : public QObject
{
    Q_OBJECT
public:
    explicit AssistantChannel(QObject *parent = nullptr);
    ~AssistantChannel() override;
signals:
    void claimUsageComplete(bool ok, const QString &msg);
    void assistantChanged();
    void modelListChanged();
public slots:
    // Assistant management
    QJsonArray getAssistantList();
    void setAssistantOrder(const QJsonArray &assistantList);
    QJsonArray getAssistantOrder();
    // 保存/读取助手侧边栏常驻显示数量。
    void setAssistantVisibleCount(int visibleCount);
    int getAssistantVisibleCount();

    void claimUsageRequest(const QString &modelId);

    // Model management
    QJsonArray getModelList(const QString &assistantId);
    bool setCurrentModel(const QString &modelId, const QString &assistantId);
    QString getCurrentModel(const QString &assistantId);

    // Writing assistant
    QString getRecentWritingDocs();
    QString getWritingTemplates();

    // Translation assistant
    QString getTranslationFAQ();

    QString getClawFAQ();

    void requestAddModel();
};

} // namespace uos_ai

#endif // ASSISTANTCHANNEL_H
