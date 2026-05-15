#ifndef WINDOWCHANNEL_H
#define WINDOWCHANNEL_H

#include <QJsonObject>
#include <QWidget>

class QWebEngineView;

namespace uos_ai {

class WindowChannel : public QObject
{
    Q_OBJECT
public:
    explicit WindowChannel(QObject *parent = nullptr);
    inline void setWebView(QWebEngineView *view) {
        webView = view;
    }
    inline void setWindow(QWidget *win) {
        if (window) {
            window->removeEventFilter(this);
        }
        window = win;
        if (window) {
            window->installEventFilter(this);
        }
    }
signals:
    void windowStateChanged(int state);
    void windowModeChanged(int mode);
    void windowAppendPrompt(const QString &question, bool isSend);
    void windowOverrideQuestion(const QString &question);
    void windowChangeToDigitalMode();
    void windowFontChanged(const QString &fontInfo);
    void toastRequested(const QString &type, const QString &message);

public slots:
    void minimize();
    void maximize();
    void restore();
    void close();
    void startMove(int sx, int sy, int tx, int ty);
    void systemMenu();
    void switchMode(int mode);
    int windowMode() ;
    void ensureMinimumWidth(int width);
    void saveMainWindowSidebarState(int width, bool expanded);
    QJsonObject getMainWindowSidebarState();
    void showAboutWindow();
    void showHelpWindow();

    // 1 - 模型,  2 - 知识库
    void showConfig(int page);

    /**
     * @brief 主窗口是否为前台活动窗口
     */
    bool isMainWindowActive();

    /**
     * @brief 显示 Toast 提示
     */
    void showToast(const QString &type, const QString &message);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int persistedMainWindowSidebarWidth();
    bool persistedMainWindowSidebarExpanded();

    QWidget *window = nullptr;
    QWebEngineView *webView = nullptr;
};

}
#endif // WINDOWCHANNEL_H
