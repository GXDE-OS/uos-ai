#ifndef LOCALMODELSERVER_H
#define LOCALMODELSERVER_H

#include <QObject>
#include <QDBusInterface>

static constexpr char PLUGINSNAME[] = "uos-ai-rag";
static constexpr char UOSAIAGENTNAME[] = "uos-ai-agent";

class LocalModelServer : public QObject
{
    Q_OBJECT

public:
    static LocalModelServer &getInstance();
    virtual ~LocalModelServer();

    void openInstallWidget(const QString &appname);
    void openManagerWidget();
    void localModelStatusChanged(const QString &app, bool isExist);
    bool checkInstallStatus(const QString &appName);
    void openInstallWidgetOnTimer(const QString &appname);

private:
    explicit LocalModelServer(QObject *parent = nullptr);
    QDBusInterface *appStoreInterface = nullptr;

signals:
    void localLLMStatusChanged(bool isExist);
    void modelPluginsStatusChanged(bool isExist);
    void sigToLaunchMgmtNoShow();
    void sigToLaunchTimer(int count);

};

#endif // LOCALMODELSERVER_H
