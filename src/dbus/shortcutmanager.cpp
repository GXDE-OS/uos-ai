#include "shortcutmanager.h"
#include "esystemcontext.h"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusArgument>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

using namespace uos_ai;

#ifdef COMPILE_ON_V25
#define KEYBINDING_SERVICE      "org.deepin.dde.Keybinding1"
#define KEYBINDING_PATH         "/org/deepin/dde/Keybinding1"
#define KEYBINDING_INTERFACE    "org.deepin.dde.Keybinding1"
#else
#define KEYBINDING_SERVICE      "com.deepin.daemon.Keybinding"
#define KEYBINDING_PATH         "/com/deepin/daemon/Keybinding"
#define KEYBINDING_INTERFACE    "com.deepin.daemon.Keybinding"
#endif

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
    , m_shortcutDbus(nullptr)
{
    m_shortcutDbus = new QDBusInterface(KEYBINDING_SERVICE,
                                       KEYBINDING_PATH,
                                       KEYBINDING_INTERFACE,
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

    if (ESystemContext::isTreeland()) {
        // V25 treeland 环境返回 D-Bus struct 数组: a(ssias)
        // struct: { Id, Name, Type, Accels[], Description }
        shortcuts = parseShortcutResults(msg.arguments().at(0));
    } else {
        // 旧版返回 JSON 字符串
        QString info = msg.arguments().at(0).toString();
        shortcuts = parseShortcutResults(info);
    }

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

QList<ShortcutInfo> ShortcutManager::parseShortcutResults(const QVariant &structData)
{
    QList<ShortcutInfo> shortcuts;

    const QDBusArgument arg = structData.value<QDBusArgument>();

    arg.beginArray();
    while (!arg.atEnd()) {
        ShortcutInfo shortcut;
        arg.beginStructure();

        // struct: { Id, Name, Type, Accels[], Description }
        QString id, name, description;
        int type;
        arg >> id >> name >> type;

        shortcut.id = id;
        shortcut.name = name;
        shortcut.type = type;

        // 解析 Accels 数组: a(s), struct 内直接在 arg 上操作
        arg.beginArray();
        if (!arg.atEnd()) {
            arg >> shortcut.accel;
        }
        arg.endArray();

        arg >> description;
        shortcut.description = description;

        arg.endStructure();
        shortcuts.append(shortcut);
    }
    arg.endArray();

    qCDebug(logDBus) << "Parsed V25" << shortcuts.size() << "shortcuts";
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
