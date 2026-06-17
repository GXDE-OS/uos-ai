#ifndef SYSTEMCHANNEL_H
#define SYSTEMCHANNEL_H

#include "utils/util.h"

#include <DGuiApplicationHelper>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QVariantList>
#include <QVariantMap>
#include "dbus/shortcutmanager.h"

class QDBusInterface;

namespace uos_ai {

/**
 * @brief SystemChannel for system resources and settings access
 *
 * Provides system-level functionality such as:
 * - Icon/theme resources
 * - System settings
 * - Other system services
 */
class SystemChannel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString activeColor READ getActiveColor NOTIFY activeColorChanged)
    Q_PROPERTY(QString fontInfo READ fontInfo NOTIFY fontChanged)
    Q_PROPERTY(int themeColor READ themeColor NOTIFY themeColorChanged)
    Q_PROPERTY(bool networkStatus READ isNetworkAvailable NOTIFY networkChanged)
public:
    enum CopyDataType {
        CopyText,
        CopyImage,
        CopyHtml
    };
    Q_ENUM(CopyDataType)

    explicit SystemChannel(QObject *parent = nullptr);
    ~SystemChannel() override;
signals:
    void openFileComplete(const QString &filePath, bool success);
    void activeColorChanged(const QString &color);
    void networkChanged(bool isOnline);
    void themeColorChanged(int value);
    void themeIconChanged();
    void fontChanged(const QString fontInfo);
    void notificationActionInvoked(const QString &actionId);
    void appUpdateAvailable(const QJsonObject &info);
public slots:
    /**
     * @brief Get icon from system theme and convert to base64
     * @param iconName System theme icon name (e.g., "uos-ai-assistant", "window-close")
     * @param width Icon width in pixels, default 64
     * @param height Icon height in pixels, default 64
     * @return base64 encoded icon data (format: "data:image/png;base64,xxx") or empty string on failure
     */
    QString getIconBase64(const QString &iconName, int width = 64, int height = 64);

    QJsonObject loadTranslations();

    // 是否中文环境
    bool checkChineseLanguage();

    // 是否启用高级CSS特性（如backdrop-filter等）
    bool isEnableAdvancedCssFeatures();

    /**
     * @brief Get system theme active (highlight) color
     * @return Hex color string (e.g., "#0081ff")
     */
    QString getActiveColor() const;

    /**
     * @brief Copy content to clipboard (text or image)
     * @param data Content to copy (text string or image path)
     * @param type Type of content to copy (CopyText, CopyImage or CopyHtml)
     */
    void copyToClipboard(const QString &data, int type);

    /**
     * @brief Copy HTML content to clipboard (supports mixed text and images)
     * @param html HTML string with inline base64 images
     */
    void copyHtmlToClipboard(const QString &html);

    Q_DECL_DEPRECATED void copyReplyText(const QString &reply);

    Q_DECL_DEPRECATED void copyImage2Clipboard(const QString &imagePath);

    /**
     * @brief Read an image file and return base64 encoded data
     * @param imagePath Image path (supports qrc://, file://, and absolute local paths)
     * @return base64 encoded image data (without data URI prefix), or empty string on failure
     */
    QString readImageAsBase64(const QString &imagePath);

    /**
     * @brief Save image to file (supports base64 data or local file path)
     * @param imageData base64 encoded image data, or local file path (qrc://, file://, absolute)
     * @param saveAs true to show save dialog, false to save to temp directory
     * @return saved file path, or empty string on failure
     */
    QString saveImageAs(const QString &imageData, bool saveAs);

    bool isNetworkAvailable();
    void openFile(const QString &filePath);
    void openUrl(const QString &url);

    // 0:Debug 1:Info 2:Warning 3:Critical
    void writeLog(int level, const QString &msg);

    bool networkAvailable();
    QString fontInfo();
    int themeColor();

    void switchThemeColor(int tc);
    int themeColorOption();

    /**
     * @brief 更新系统音量
     * @param percent 音量百分比 (0-100)
     */
    void updateVolume(int percent);

    /**
     * @brief 更新屏幕亮度
     * @param percent 亮度百分比 (0-100)
     */
    void updateBrightness(int percent);

    /**
     * @brief 打开控制中心并定位到指定模块
     * @param module 模块名称 (如 "sound", "display", "appearance" 等)
     */
    void openControlCenter(const QString &module);

    /**
     * @brief 更新系统字号
     * @param size 字号值 (9-20)
     */
    void updateFontSize(float size);

    /**
     * @brief 切换护眼模式
     * @param enabled 是否启用护眼模式
     */
    void toggleEyesProtection(bool enabled);

    /**
     * @brief 切换蓝牙
     * @param enabled 是否启用蓝牙
     */
    void doBluetoothConfig(bool enabled);

    /**
     * @brief 切换勿扰模式
     * @param enabled 是否启用勿扰模式
     */
    void doNoDisturb(bool enabled);

    /**
     * @brief 切换WiFi
     * @param enabled 是否启用WiFi
     */
    void switchWifi(bool enabled);

    /**
     * @brief 打开应用商店并定位到指定应用
     * @param appPackage 应用包名
     */
    void openAppStore(const QString &appPackage);

    /**
     * @brief 打开应用商店并定位到指定页
     * @param tabName 页名称
     */
    void openAppStoreTab(const QString &tabName);

    /**
     * @brief 打开日历应用
     * @param subject 日程主题
     * @param startTime 开始时间
     * @param endTime 结束时间
     */
    void openCalendar(const QString &subject, const QString &startTime, const QString &endTime);

    // 查询快捷键
    QString getCurrentShortcut();
    QString getCurrentTalkShortcut();  // 查询数字人快捷键

    /**
     * @brief 发送 Freedesktop 通知
     * @param title 标题
     * @param body 正文
     * @param icon 图标名或图标路径
     * @param actions 操作列表，支持：
     *        1) [{ key: "view_now", text: "立即查看" }, ...]
     *        2) ["view_now", "立即查看", "icomp:bash_approved:id:conv:msg", "Allow"]
     * @param timeoutMs 持续时间（毫秒），-1 表示服务端默认
     */
    void notify(const QString &title,
                const QString &body,
                const QString &icon = QString(),
                const QVariantList &actions = QVariantList(),
                int timeoutMs = -1);

    /**
     * @brief 前端窗口创建后主动触发应用更新检查。
     */
    void checkAppUpdate();

    /**
     * @brief 标记指定更新版本的提醒已被消费。
     */
    void markAppUpdateReminderConsumed(const QString &availableVersion);

private slots:
    void onThemeTypeChanged(DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType themeType);
    void onFontChanged(const QFont &);
    void onNotificationActionInvoked(uint notificationId, const QString &actionKey);
public slots:
    void emitNotificationAction(const QString &actionId);
private:
    void searchShortCut();
    QStringList normalizeNotificationActions(const QVariantList &actions) const;
    QVariantMap buildNotificationHints(const QStringList &normalizedActions) const;

private:
    QString m_activeColor;
    int m_themeColor = 1;
    QTemporaryDir m_ttpDir;
    ShortcutInfo m_uosAiShortcut;
    ShortcutInfo m_uosAiTalkShortcut;
    QScopedPointer<QDBusInterface> m_notificationProxy;
};

} // namespace uos_ai

#endif // SYSTEMCHANNEL_H
