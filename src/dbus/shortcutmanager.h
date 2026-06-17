#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H


#include <QObject>
#include <QDBusInterface>
#include <QList>
#include <QVariant>

namespace uos_ai {
// 快捷键信息结构体
struct ShortcutInfo {
    QString id;         // 旧版: 名称(如"UOS AI"); V25 treeland 环境: 名称(如"UOS AI"), 用于匹配
    QString name;       // 快捷键显示名称
    QString exec;       // 执行命令 (仅旧版 JSON 返回)
    QString accel;      // 快捷键绑定 (如"<Super>Space")
    int type = 0;       // 快捷键类型 (仅V25 treeland 环境返回)
    QString description; // 描述信息 (仅V25 treeland 环境返回)

    bool isEmpty() const { return id.isEmpty(); }
    void clear() { id.clear(); name.clear(); exec.clear(); accel.clear(); type = 0; description.clear(); }
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
    QList<ShortcutInfo> parseShortcutResults(const QString &jsonData);   // 旧版: JSON 字符串
    QList<ShortcutInfo> parseShortcutResults(const QVariant &structData);  // V25 treeland 环境: D-Bus struct 数组

private:
    QDBusInterface *m_shortcutDbus;

    // 禁用拷贝构造和赋值
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;
};
}

#endif // SHORTCUTMANAGER_H 
