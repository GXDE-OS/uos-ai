#ifndef SIDEWINDOW_H
#define SIDEWINDOW_H

#include "appwindow.h"

namespace uos_ai {

class SideWindow : public AppWindow
{
    Q_OBJECT
public:
    explicit SideWindow(QWidget *parent = nullptr);
    void initWindow(WebContext *ctx) override;
    WindowMode mode() const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
public slots:
    void splitWindow();
};

}
#endif // SIDEWINDOW_H
