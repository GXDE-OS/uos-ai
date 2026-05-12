#ifndef APPWEBPAGE_H
#define APPWEBPAGE_H

#include <QWebEnginePage>

namespace uos_ai {

class AppWebPage : public QWebEnginePage
{
public:
    explicit AppWebPage(QObject *parent = nullptr);
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

};

}
#endif // APPWEBPAGE_H
