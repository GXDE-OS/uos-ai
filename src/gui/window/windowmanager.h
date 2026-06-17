#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include "global_define.h"

#include <QObject>
#include <QPair>
#include <QList>
#include <QMap>

namespace uos_ai {

class AppWindow;
class WebContext;
class AboutWindow;
class UpdateLogDialog;

class WindowManager : public QObject
{
    Q_OBJECT
public:
    static WindowManager *instance();
    void createContext();
    void showWindow(WindowMode mode);
    void hideWindow();
    WindowMode currentMode() const;
    bool isWindowExist();
    bool isWindowVisible();
    bool isWindowHidden();
    bool isActiveWindow();
    void showAboutWindow();
    void showUpdateLogWindow();
    WebContext* context() const;
private slots:
    void handleScreenshotRequested();
    void handleScreenshotDone(const QString &imagePath);
    void handleScreenshotCanceled();
    void handleToastRequested(const QString &type, const QString &message);

private:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager();
private:
    AppWindow *curWindow = nullptr;
    AboutWindow *aboutWindow = nullptr;
    UpdateLogDialog *updateLogWindow = nullptr;
    WebContext *ctx = nullptr;
    bool m_wasMaximized = false;
};

}

#define WmIns WindowManager::instance

#endif // WINDOWMANAGER_H
