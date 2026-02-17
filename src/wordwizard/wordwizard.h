#ifndef WORDWIZARD_H
#define WORDWIZARD_H
#include "uosai_global.h"
#include "private/disableappwidget.h"

#include <QVariant>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>

namespace uos_ai {

class CustomFunction
{
public:
    QString name; // 功能名称（自定义功能使用）
    QString prompt; // 功能提示词（自定义功能使用）
    bool isCustom = false; // 是否自定义
    bool isHidden = false; // 是否隐藏（默认功能使用）
    int defaultFunctionType = 0; // 默认功能类型（默认功能使用）

    // 占位符，用于替换划词内容
    static const QString kPlaceholder;

    CustomFunction() {}

    CustomFunction(const QString &name, const QString &prompt, bool isCustom = false, bool isHidden = false, int defaultFunctionType = 0)
            : name(name.left(20)), prompt(prompt), isCustom(isCustom), isHidden(isHidden), defaultFunctionType(defaultFunctionType) {}

    CustomFunction(const QJsonObject &json) {
        name = json["name"].toString().left(20); // 限制名称长度为20个字符
        prompt = json["prompt"].toString();
        isCustom = json["isCustom"].toBool();
        isHidden = json["isHidden"].toBool();
        defaultFunctionType = json["defaultFunctionType"].toInt();
    }

    QJsonObject toJson() const {
        QJsonObject json;
        json["name"] = name;
        json["prompt"] = prompt;
        json["isCustom"] = isCustom;
        json["isHidden"] = isHidden;
        json["defaultFunctionType"] = defaultFunctionType;
        return json;
    }

    bool operator==(const CustomFunction &other) const {
        return name == other.name && prompt == other.prompt && isCustom == other.isCustom && isHidden == other.isHidden && defaultFunctionType == other.defaultFunctionType;
    }
};

class BaseClipboard;
class BaseMonitor;
class WizardWrapper;
class AiWriterDialog;
class InputWindow;  // 新增：前向声明
class WordWizard : public QObject
{
    Q_OBJECT
public:
    explicit WordWizard(QObject *parent = nullptr);
    // 初始化自定义功能列表
    void initCunstomFunctions();
    void initWordWizard();
    void initConnect();
    bool queryHiddenStatus();
    // 保存自定义功能列表
    static void saveCustomFunctions();
    ~WordWizard();

    static bool launchUosBrowser(QString url);
    static bool launchDefaultBrowser(QString url);
    static bool onSearchBtnClicked(const QString &text);
    static void onInsertBtnClicked(QString text) {}
    static void onReplaceBtnClicked(QString text) {}
    static bool fcitxWritable() { return kIsFcitxWritable; }
    static bool doAddToKnowledgeBase(const QString &text);

    static QString getDefaultSkillName(int defaultFunctionType);
    static QIcon getDefaultSkillIcon(int defaultFunctionType);

    // 自定义功能列表
    static QList<CustomFunction> kCustomFunctionList;

public:
    enum WizardType {
        WIZARD_TYPE_SEARCH = 0, // 搜索
        WIZARD_TYPE_EXPLAIN, // 解释
        WIZARD_TYPE_SUMMARIZE, // 总结
        WIZARD_TYPE_TRANSLATE, // 翻译
        WIZARD_TYPE_RENEW, // 续写
        WIZARD_TYPE_EXTEND, // 扩写
        WIZARD_TYPE_CORRECT, // 纠错
        WIZARD_TYPE_POLISH, // 润色
        WIZARD_TYPE_KNOWLEDGE, // 添加到知识库
        WIZARD_TYPE_MAX = WIZARD_TYPE_KNOWLEDGE,
        WIZARD_TYPE_CUSTOM, //自定义设置
        WIZARD_TYPE_HIDDEN, // 隐藏/禁用
    };

signals:
    /**
     * @brief 显示快捷面板
     * @param type 处理类别
     * @param query 文本
     * @param pos 显示位置
     */
    void sigToLaunchAiQuick(int type, QString query, QPoint pos, bool isCustom);
    void sigToCloseAiQuick();
    void sigToLaunchMgmt(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString &locateTitle);
    void signalHiddenwidget(bool isHidden);
    void signalAddDisabledApp(const QString &appName);

public slots:
    void onGlobalMousePress(int x, int y);
    void onGlobalMouseRelease(int x, int y);
    void onKeyEscapePress();
    void onShowScribeWord();
    void onShortcutPressed();
    void onShortcutTranslate();
    void onFunctionTriggered(int wizardtype, const QPoint &cursorPos, bool isCustom);
    void onSaveClipText();
    void onCloseBtnClicked();
    void onChangeXEventMonitorStatus(bool isopen);
    void onHiddenwidget(bool ishide) { m_isHidden = ishide; }
    void onChangeHiddenStatus(bool isHidden);
    void onFocusIn() { kIsFcitxWritable = true; }
    void onFocusOut() { kIsFcitxWritable = false; }
    void onDisableInApp();
    void onDisableInProcess();
    void updateDisabledApps(const QStringList &appList);
    void onShowInputWindow(const QPoint &pos, const QRect &screenRect, int wizardWidth, int wizardHeight);
    static void onIconBtnClicked(QString text = "");
    void onWebViewLoadFinished() { m_isWebviewOk = true; }

private:
    void onHiddenActionTriggered();
    void getScreenRect();
    void adjustMulScreenPos(QPoint &pos);
    bool isAppDisabled(const QString &appName);
    bool isProcessDisabled(int pid, const QString &appName);
    void checkDisabledProcesses();

    static volatile bool kIsFcitxWritable;
    volatile bool m_isWebviewOk = true;

    double m_scaleFactor; // 屏幕缩放比
    int m_mouseClickX;
    int m_mouseClickY;
    int m_mouseReleaseX;
    int m_mouseReleaseY;
    bool m_isMousePress = false;
    bool m_isMouseRelease = false;

    bool m_stopShow = false;//close x11 event monitor
    bool m_isClose = false;//close btn, one time
    bool m_isHidden = false;//hidden action
    bool m_isShortcut = false;

    QPoint m_Point;
    QString m_clipText = "";

    BaseClipboard *m_selectclip = nullptr;
    WizardWrapper *m_selectwid = nullptr;
    AiWriterDialog *m_writerwid = nullptr;
    InputWindow *m_inputWindow = nullptr;
    QThread *m_eventMonitor = nullptr;

    QPoint m_cursorPos;
    QRect m_screenRect;

    QTimer *m_timer = nullptr;
    QTimer *m_processCheckTimer = nullptr;

    QString m_curApp;
    QStringList m_disabledApps; // 存储禁用的应用列表
    int m_curPid;
    QMap<int, QString> m_disabledProcess; // 存储禁用的进程列表
};
}
#endif // WORDWIZARD_H
