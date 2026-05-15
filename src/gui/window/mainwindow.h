#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "appwindow.h"

#include <QSize>

class QCloseEvent;
class QResizeEvent;
class QTimer;

namespace uos_ai {

class MainWindow : public AppWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void initWindow(WebContext *ctx) override;
    WindowMode mode() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void restorePersistedSize();
    void flushPendingWindowSize();
    QSize persistedMainWindowSize();
    void saveMainWindowSize(const QSize &size);

    QTimer *m_persistSizeTimer = nullptr;
    QSize m_pendingWindowSize;
};

}

#endif // MAINWINDOW_H
