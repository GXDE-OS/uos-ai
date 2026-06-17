#ifndef ONLINESEARCHAGENT_H
#define ONLINESEARCHAGENT_H

#include "llmagent.h"

#include <QObject>
#include <QMutex>
#include <QWebEnginePage>

namespace uos_ai {

class HeadlessWebEnginePage : public QWebEnginePage
{
public:
    using QWebEnginePage::QWebEnginePage;
    ~HeadlessWebEnginePage() override;
protected:
    QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes) override;
    QWebEnginePage *createWindow(WebWindowType type) override;

    void javaScriptAlert(const QUrl &securityOrigin, const QString &msg) override;

    bool javaScriptConfirm(const QUrl &securityOrigin, const QString &msg) override;

    bool javaScriptPrompt(const QUrl &securityOrigin, const QString &msg,
                        const QString &defaultValue, QString *result) override;
};

class WebSearchContext : public QObject
{
    Q_OBJECT
public:
    explicit WebSearchContext(QObject *parent = nullptr);
    ~WebSearchContext() override;
    QString getContent();
    void setJs(const QString &text);
signals:
    void finished();
public slots:
    void fetchContent(const QString &url);
protected:
    QWebEnginePage* createWebPage();
private:
    QMutex mtx;
    QString content;
    QString js;
};

class OnlineSearchAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit OnlineSearchAgent(QObject *parent = nullptr);
    ~OnlineSearchAgent() override;

    bool initialize() override;

    QString systemPrompt() const override;

    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &agentParams = {}) override;

    void cancel() override;
signals:
    void agentCancled();
protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    /**
     * 创建工具定义
     */
    void createTools();

    /**
     * 执行网络搜索
     * @param query 搜索关键词
     * @return 搜索结果JSON字符串
     */
   QPair<int, QString> performWebSearch(const QString &query, QJsonArray *page = nullptr);

    QString searchContent;
    QString readability;
    int maxContentSize = 8000 * 1.5; // 8000字符，换算token乘以1.5，粗略估算token
};

}

#endif // ONLINESEARCHAGENT_H
