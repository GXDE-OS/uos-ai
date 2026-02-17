#include "dragmonitor.h"
#include <QLoggingCategory>
#include <QDBusConnection>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

using namespace uos_ai;
DragMonitor::DragMonitor(QObject *parent)
    : QObject{parent}
{
    auto con = QDBusConnection::sessionBus();
    bool fileManagerConnected = con.connect("org.deepin.filemanager.drag",
                "/org/deepin/filemanager/drag", "org.deepin.filemanager.drag", "DragEnter",
                this, SLOT(onDragNotify(QStringList)));
    
    bool desktopConnected = con.connect("com.deepin.dde.desktop",
                "/org/deepin/dde/desktop/canvas", "org.deepin.dde.desktop.canvas", "DragEnter",
                this, SLOT(onDragNotify(QStringList)));

    qCDebug(logAIBar) << "DragMonitor initialized - FileManager connection:" << fileManagerConnected 
                           << "Desktop connection:" << desktopConnected;
}

void DragMonitor::onDragNotify(const QStringList &urls)
{
    qCDebug(logAIBar) << "Drag event detected with" << urls.size() << "items";
    if (!urls.isEmpty()) {
        qCDebug(logAIBar) << "First dragged item:" << urls.first();
    }
    emit dragEnter(urls);
}
