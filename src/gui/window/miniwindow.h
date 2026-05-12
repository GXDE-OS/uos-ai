#ifndef MINIWINDOW_H
#define MINIWINDOW_H

#include "appwindow.h"

class DDeDockObject;

namespace uos_ai {

class MiniWindow : public AppWindow
{
    Q_OBJECT
public:
    explicit MiniWindow(QWidget *parent = nullptr);
    void initWindow(WebContext *ctx) override;
    WindowMode mode() const override;
public slots:
    void updateGeometry();
private:
    QRect findDisplayScreenContaining(const QRect &dockRect, qreal devicePixelRatio) const;
    int calculateMaxHeight(const QRect &displayRect, const QRect &dockRect,
                          DockPosition dockPosition, int displayMode,
                          qreal devicePixelRatio, int margin) const;
    QPoint calculateWindowPosition(const QRect &displayRect, const QRect &dockRect,
                                  DockPosition dockPosition, int displayMode,
                                  qreal devicePixelRatio, int windowWidth, int margin) const;
private:
    DDeDockObject *dockApi = nullptr;
};

}

#endif // MINIWINDOW_H
