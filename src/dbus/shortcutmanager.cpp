#include "shortcutmanager.h"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

UOSAI_USE_NAMESPACE

#define KEYBINDING_SERVICE      "com.deepin.daemon.Keybinding"
#define KEYBINDING_PATH         "/com/deepin/daemon/Keybinding"
#define KEYBINDING_INTERFACE    "com.deepin.daemon.Keybinding"

#define KEYBINDING_SERVICE_ALT  "org.deepin.dde.Keybinding1"
#define KEYBINDING_PATH_ALT     "/org/deepin/dde/Keybinding1"
#define KEYBINDING_INTERFACE_ALT "org.deepin.dde.Keybinding1"

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
    , m_shortcutDbus(nullptr)
{
    QString serviceName = KEYBINDING_SERVICE;
    QString path = KEYBINDING_PATH;
    QString interface = KEYBINDING_INTERFACE;
    
    // 检查主服务是否已注册，如果未注册则使用备用服务
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(KEYBINDING_SERVICE)) {
        qCWarning(logDBus) << "Service" << KEYBINDING_SERVICE << "not registered, trying" << KEYBINDING_SERVICE_ALT;
        serviceName = KEYBINDING_SERVICE_ALT;
        path = KEYBINDING_PATH_ALT;
        interface = KEYBINDING_INTERFACE_ALT;
    }
    
    m_shortcutDbus = new QDBusInterface(serviceName,
                                       path,
                                       interface,
                                       QDBusConnection::sessionBus(),
                                       this);
}

ShortcutManager::~ShortcutManager()
{
    if (m_shortcutDbus) {
        delete m_shortcutDbus;
    }
}

ShortcutManager& ShortcutManager::getInstance()
{
    static ShortcutManager instance;
    return instance;
}

bool ShortcutManager::isValid() const
{
    return m_shortcutDbus && m_shortcutDbus->isValid();
}

QList<ShortcutInfo> ShortcutManager::searchShortcuts(const QString &keyword)
{
    QList<ShortcutInfo> shortcuts;
    
    if (!isValid()) {
        qCWarning(logDBus) << "Invalid D-Bus interface for shortcuts";
        return shortcuts;
    }

    qCDebug(logDBus) << "Searching shortcuts with keyword:" << keyword;
    
    QDBusMessage msg = m_shortcutDbus->call("SearchShortcuts", keyword);
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to search shortcuts:" << msg.errorMessage();
        return shortcuts;
    }
    
    if (msg.arguments().isEmpty()) {
        qCWarning(logDBus) << "Empty response from SearchShortcuts";
        return shortcuts;
    }
    
    QString info = msg.arguments().at(0).toString();
    shortcuts = parseShortcutResults(info);
    
    return shortcuts;
}

QList<ShortcutInfo> ShortcutManager::parseShortcutResults(const QString &jsonData)
{
    QList<ShortcutInfo> shortcuts;
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData.toUtf8());
    if (!jsonDoc.isArray()) {
        qCWarning(logDBus) << "Invalid JSON format for shortcuts";
        return shortcuts;
    }
    
    QJsonArray jsonArray = jsonDoc.array();
    
    for (const QJsonValue &value : jsonArray) {
        QJsonObject jsonObj = value.toObject();
        ShortcutInfo shortcut;
        shortcut.id = jsonObj.value("Id").toString();
        shortcut.name = jsonObj.value("Name").toString();
        shortcut.exec = jsonObj.value("Exec").toString();
        
        QJsonArray accelsArray = jsonObj.value("Accels").toArray();
        shortcut.accel = accelsArray.isEmpty() ? QString() : accelsArray.at(0).toString();
        
        shortcuts.append(shortcut);
    }
    
    qCDebug(logDBus) << "Parsed" << shortcuts.size() << "shortcuts";
    return shortcuts;
}

bool ShortcutManager::addCustomShortcut(const QString &name, const QString &exec, const QString &accel)
{
    if (!isValid()) {
        qCWarning(logDBus) << "Invalid D-Bus interface, cannot add shortcut";
        return false;
    }

    QDBusMessage reply = m_shortcutDbus->call("AddCustomShortcut", name, exec, accel);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to add custom shortcut:" << reply.errorMessage();
        return false;
    }
    
    qCDebug(logDBus) << "Successfully added shortcut:" << name << accel;
    return true;
}

bool ShortcutManager::modifyCustomShortcut(const QString &id, const QString &name, const QString &exec, const QString &accel)
{
    if (!isValid()) {
        qCWarning(logDBus) << "Invalid D-Bus interface, cannot modify shortcut";
        return false;
    }

    QDBusMessage reply = m_shortcutDbus->call("ModifyCustomShortcut", id, name, exec, accel);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to modify custom shortcut:" << reply.errorMessage();
        return false;
    }
    
    qCDebug(logDBus) << "Successfully modified shortcut:" << id << "to" << accel;
    return true;
}

bool ShortcutManager::deleteCustomShortcut(const QString &id)
{
    if (!isValid()) {
        qCWarning(logDBus) << "Invalid D-Bus interface, cannot delete shortcut";
        return false;
    }

    QDBusMessage reply = m_shortcutDbus->call("DeleteCustomShortcut", id);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logDBus) << "Failed to delete custom shortcut:" << reply.errorMessage();
        return false;
    }
    
    qCDebug(logDBus) << "Successfully deleted shortcut:" << id;
    return true;
} 
