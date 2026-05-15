#include "appwebview.h"

#ifdef COMPILE_ON_QT6
#include <QWebEngineUrlScheme>
#endif

#include <QWebEngineSettings>
#include <QDropEvent>
#include <QLoggingCategory>
#include <QMimeData>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

AppWebView::AppWebView(QWidget *parent) : QWebEngineView(parent)
{
#ifdef COMPILE_ON_QT6
    //解决qt6浏览器跨域问题
    QWebEngineUrlScheme qrcSchema(QByteArrayLiteral("qrc"));
    qrcSchema.setFlags(QWebEngineUrlScheme::ViewSourceAllowed|QWebEngineUrlScheme::LocalAccessAllowed);
    QWebEngineUrlScheme::registerScheme(qrcSchema);
    qCDebug(logAIGUI) << "Registered QRC URL scheme for Qt6";
#endif

    // 允许本地内容访问文件 URLs，解决域外资源加载问题
    QWebEngineSettings *settings = page()->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    qCDebug(logAIGUI) << "Enabled local file URL access and remote URL access";

    setContextMenuPolicy(Qt::NoContextMenu);
    setAcceptDrops(true);
}

void AppWebView::dropEvent(QDropEvent *event)
{
    if (!event) {
        return;
    }

    if (!event->mimeData()) {
        QWebEngineView::dropEvent(event);
        return;
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    QStringList paths {};
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            paths << url.toLocalFile();
        } else if (!url.path().isEmpty()) {
            paths << url.path();
        }
    }

    if (paths.isEmpty()) {
        QWebEngineView::dropEvent(event);
        return;
    }

#ifdef COMPILE_ON_QT6
    const QPoint pos = event->position().toPoint();
#else
    const QPoint pos = event->pos();
#endif

    event->acceptProposedAction();
    emit nativeFilesDropped(paths, pos.x(), pos.y());
}
