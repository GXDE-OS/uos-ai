#ifndef LOCALMODELSERVER_H
#define LOCALMODELSERVER_H

#include <QObject>
#include <QDBusInterface>

inline constexpr char PLUGINSNAME[] = "uos-ai-rag";
inline constexpr char UOSAIAGENTNAME[] = "uos-ai-agent";

class LocalModelServer : public QObject
{
    Q_OBJECT

public:
    static LocalModelServer &getInstance();
    virtual ~LocalModelServer();

    void openInstallWidget(const QString &appname);
    void openManagerWidget();
    bool checkInstallStatus(const QString &appName);
    void openInstallWidgetOnTimer(const QString &appname);

private:
    explicit LocalModelServer(QObject *parent = nullptr);
    QDBusInterface *appStoreInterface = nullptr;

signals:
    void pluginStatusChanged(const QString &app, bool isExist);
    void beginCheck(const QString &name, int count);

};

#endif // LOCALMODELSERVER_H
