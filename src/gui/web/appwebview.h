#ifndef APPWEBVIEW_H
#define APPWEBVIEW_H

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QStringList>

class QDropEvent;

namespace uos_ai {

class AppWebView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit AppWebView(QWidget *parent = nullptr);

signals:
    void nativeFilesDropped(const QStringList &paths, int x, int y);

protected:
    void dropEvent(QDropEvent *event) override;
};

}

#endif // APPWEBVIEW_H
