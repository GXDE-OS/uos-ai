#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "global_define.h"

#include <QMainWindow>

namespace uos_ai {

class WebContext;
class AppWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AppWindow(QWidget *parent = nullptr);
    virtual void initWindow(WebContext *ctx);
    virtual WindowMode mode() const = 0;
 protected:
    WebContext *webCtx = nullptr;
};

}


#endif // APPWINDOW_H
