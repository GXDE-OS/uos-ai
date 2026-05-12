#include "appwebpage.h"

#include <QDesktopServices>
#include <QColor>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

AppWebPage::AppWebPage(QObject *parent) : QWebEnginePage(parent)
{
    setBackgroundColor(QColor(Qt::transparent));
}

bool AppWebPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    qCInfo(logAIGUI) << "navigation request" << url << type << isMainFrame;
    if (type == QWebEnginePage::NavigationTypeLinkClicked) {
        // Open the link using system's default browser
        QDesktopServices::openUrl(url);
        return false;
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}
