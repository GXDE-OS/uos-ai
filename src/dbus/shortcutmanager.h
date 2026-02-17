#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H
#include "uosai_global.h"

#include <QObject>
#include <QDBusInterface>
#include <QList>

namespace uos_ai {
// 快捷键信息结构体
struct ShortcutInfo {
    QString id;
    QString name;
    QString exec;
    QString accel;
    
    bool isEmpty() const { return id.isEmpty(); }
    void clear() { id.clear(); name.clear(); exec.clear(); accel.clear(); }
};

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    static ShortcutManager& getInstance();
    
    // 查询快捷键信息
    QList<ShortcutInfo> searchShortcuts(const QString &keyword);
    
    // 添加自定义快捷键
    bool addCustomShortcut(const QString &name, const QString &exec, const QString &accel);
    
    // 修改自定义快捷键
    bool modifyCustomShortcut(const QString &id, const QString &name, const QString &exec, const QString &accel);
    
    // 删除自定义快捷键
    bool deleteCustomShortcut(const QString &id);
    
    // 检查 D-Bus 接口是否有效
    bool isValid() const;

private:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager();
    
    // 解析快捷键查询结果
    QList<ShortcutInfo> parseShortcutResults(const QString &jsonData);

private:
    QDBusInterface *m_shortcutDbus;

    // 禁用拷贝构造和赋值
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;
};
}

#endif // SHORTCUTMANAGER_H 
