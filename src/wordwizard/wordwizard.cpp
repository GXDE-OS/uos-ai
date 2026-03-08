#include <gio/gio.h>

#include "wordwizard.h"
#include "llmutils.h"
#include "private/xeventmonitor.h"
#ifdef COMPILE_ON_V25
#include "treelandclipboard.h"
#else
#include "private/xclipboard.h"
#endif
#include "wrapper/wizardwrapper.h"
#include "wrapper/inputwindow.h"
#include "dbs/knowledgebasemanager.h"
#include "serverwrapper.h"
#include "fcitxinputserver.h"
#include "gui/aiwriterdialog.h"
#include <gui/chat/private/eaiexecutor.h>
#include "dconfigmanager.h"
#include "util.h"
#include "esystemcontext.h"
#include <report/followfunctionpoint.h>
#include <report/knowledgefunctionpoint.h>
#include <report/eventlogutil.h>
#include <apputils.h>
#include <application.h>

#include <QApplication>
#include <QScreen>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDesktopServices>
#include <QUrl>
#include <QReadWriteLock>
#include <QTimer>

#include <DDialog>
#include <DLabel>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

volatile bool WordWizard::kIsFcitxWritable = true;

const QString CustomFunction::kPlaceholder = "${SELECTION}";
QList<CustomFunction> WordWizard::kCustomFunctionList;

void WordWizard::saveCustomFunctions()
{
    QStringList jsonList;
    for (const CustomFunction &item : kCustomFunctionList) {
        jsonList.append(QJsonDocument(item.toJson()).toJson(QJsonDocument::Compact));
    }
    if (DConfigManager::checkConfigAvailable(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS))
        DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS, QVariant::fromValue(jsonList));
}

WordWizard::WordWizard(QObject *parent) : QObject(parent)
{
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    m_scaleFactor = primaryScreen->devicePixelRatio();

    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    m_timer->setSingleShot(true);

    m_processCheckTimer = new QTimer(this);
    m_processCheckTimer->setInterval(3600000); // 每1小时检查一次
    m_processCheckTimer->setSingleShot(false);

    initCunstomFunctions();
    initWordWizard();
    initConnect();
    FcitxInputServer::getInstance();

    connect(m_processCheckTimer, &QTimer::timeout, this, &WordWizard::checkDisabledProcesses);
    m_processCheckTimer->start();
}

WordWizard::~WordWizard()
{
    delete m_eventMonitor;
    m_eventMonitor = nullptr;

    disconnect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusIn, this, &WordWizard::onFocusIn);
    disconnect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusOut, this, &WordWizard::onFocusOut);
}

void WordWizard::initWordWizard()
{
    m_selectwid = &WizardWrapper::instance();

    m_selectclip = BaseClipboard::instance();
    m_eventMonitor = BaseMonitor::instance();

    m_writerwid = &AiWriterDialog::instance();
    m_inputWindow = &InputWindow::instance();

    bool isHidden = DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_ISHIDDEN).toBool();
    qCInfo(logWordWizard) << "Initial hidden status:" << isHidden;
    m_isHidden = isHidden;
    m_selectwid->isEnabledAction(!isHidden);
    m_inputWindow->isEnabledAction(!isHidden);

    QVariant disabledAppsVar = DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_DISABLED_APPS);
    if (disabledAppsVar.isValid()) {
        m_disabledApps = disabledAppsVar.toStringList();
        qCDebug(logWordWizard) << "Disabled apps list loaded:" << m_disabledApps;
    }
}

void WordWizard::initCunstomFunctions()
{
    QStringList jsonList;
    jsonList = DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_CUSTOM_FUNCTIONS).value<QStringList>();
    if (jsonList.isEmpty()) {
        qCInfo(logWordWizard) << "Initial custom functions, use default value";
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_SEARCH
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_EXPLAIN
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_SUMMARIZE
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_TRANSLATE
        });
#ifdef ENABLE_ASSISTANT
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_KNOWLEDGE
        });
#endif
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_RENEW
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_EXTEND
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_CORRECT
        });
        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            false,
            WIZARD_TYPE_POLISH
        });
        return;
    }

    qCInfo(logWordWizard) << "Initial custom functions, use config value";
    QSet<int> defaultFunctionTypeSet;
    for (const QString &jsonStr : jsonList) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qCWarning(logWordWizard) << "Invalid JSON format:" << jsonStr;
            continue;
        }

        QJsonObject json = jsonDoc.object();
        // 安全检查
        CustomFunction func(json);
#ifndef ENABLE_ASSISTANT
        if (func.defaultFunctionType == WIZARD_TYPE_KNOWLEDGE)
            continue;
#endif
        if (!func.isCustom) {
            // 默认项安全检查
            if (func.defaultFunctionType < WIZARD_TYPE_SEARCH || func.defaultFunctionType > WIZARD_TYPE_KNOWLEDGE) {
                qCWarning(logWordWizard) << "Invalid default function type:" << func.defaultFunctionType << "in JSON:" << jsonStr;
                continue;
            }

            if (defaultFunctionTypeSet.contains(func.defaultFunctionType)) {
                qCWarning(logWordWizard) << "Duplicate default function type:" << func.defaultFunctionType << "in JSON:" << jsonStr;
                continue;
            }

            defaultFunctionTypeSet.insert(func.defaultFunctionType);
        } else {
            // 自定义项安全检查
            if (func.name.isEmpty() || func.prompt.isEmpty()) {
                qCWarning(logWordWizard) << "Invalid custom function, name or prompt is empty in JSON:" << jsonStr;
                continue;
            }
        }
        kCustomFunctionList.append(func);
    }

    // 兜底操作：如果用户从DConfig删除了默认功能项，这里进行补全，并配置为隐藏
    bool isAllOk = true;
    for (int i = WIZARD_TYPE_SEARCH; i <= WIZARD_TYPE_KNOWLEDGE; i++) {
        if (defaultFunctionTypeSet.contains(i)) {
            continue;
        }
#ifndef ENABLE_ASSISTANT
        if (i == WIZARD_TYPE_KNOWLEDGE) {
            continue;
        }
#endif

        kCustomFunctionList.append(CustomFunction {
            "",
            "",
            false,
            true,
            i
        });
        isAllOk = false;
    }
    if (!isAllOk) {
        qCInfo(logWordWizard) << "Default function types are missing, add default functions";
        this->saveCustomFunctions();
    }
}

void WordWizard::initConnect()
{
    connect(m_selectclip, &BaseClipboard::selectWords, this, [ = ]{
        if (m_isMousePress) {
            if (m_isMouseRelease && m_timer->isActive()) {
                //先鼠标释放,再剪贴板内容变动
                onShowScribeWord();
                m_timer->stop();
            } else
                m_timer->start();
        }
    });
    connect(m_timer, &QTimer::timeout, this, [ = ]{
        m_isMouseRelease = false;
    });

    connect(m_selectwid, &WizardWrapper::signalFunctionTriggered, this, &WordWizard::onFunctionTriggered);
    connect(m_selectwid, &WizardWrapper::signalRequestServer, this, &WordWizard::onSaveClipText);
    connect(m_inputWindow, &InputWindow::signalFunctionTriggered, this, &WordWizard::onFunctionTriggered);
    connect(m_inputWindow, &InputWindow::signalRequestServer, this, &WordWizard::onSaveClipText);
    connect(m_selectwid, &WizardWrapper::signalCloseBtnClicked, this, &WordWizard::onCloseBtnClicked);
    connect(m_selectwid, &WizardWrapper::signalIconBtnClicked, this, [&]() {
        this->onIconBtnClicked(m_selectclip->getClipText());
        m_selectwid->close();
    });
    connect(EAiExec(), &EAiExecutor::sigWebViewLoadFinished, this, &WordWizard::onWebViewLoadFinished);
    connect(m_inputWindow, &InputWindow::signalInputTextTriggered, this, [&](const QString &inputText) {
        QString combinedText = QString("%0:%1").arg(inputText).arg(m_selectclip->getClipText());
        if (EAiExec()->showChatWindow()) { // 等待前端渲染完成后再传，下一步会接收信号判断是否渲染完成
            m_isWebviewOk = false;
            QTimer::singleShot(2000, nullptr, [&, combinedText] {
                if (m_isWebviewOk) {
                    qCInfo(logWordWizard) << "webview is ok!";
                    EAiExec()->wordWizardAskAI(combinedText, AssistantType::UOS_AI);
                }
                m_inputWindow->close();
            });
        } else {
            EAiExec()->wordWizardAskAI(combinedText, AssistantType::UOS_AI);
            m_inputWindow->close();
        }
    });
    connect(m_inputWindow, &InputWindow::signalCloseBtnClicked, this, &WordWizard::onCloseBtnClicked);

    connect(m_selectwid, &WizardWrapper::signalDisableInApp, this, &WordWizard::onDisableInApp);
    connect(m_selectwid, &WizardWrapper::signalDisableInProcess, this, &WordWizard::onDisableInProcess);
    connect(m_inputWindow, &InputWindow::signalDisableInApp, this, &WordWizard::onDisableInApp);
    connect(m_inputWindow, &InputWindow::signalDisableInProcess, this, &WordWizard::onDisableInProcess);
    connect(m_selectwid, &WizardWrapper::signalShowInputWindow, this, &WordWizard::onShowInputWindow);

    connect(m_eventMonitor, SIGNAL(mousePress(int, int)), this, SLOT(onGlobalMousePress(int, int)), Qt::QueuedConnection);
    connect(m_eventMonitor, SIGNAL(mouseRelease(int, int)), this, SLOT(onGlobalMouseRelease(int, int)), Qt::QueuedConnection);
    connect(m_eventMonitor, SIGNAL(keyEscapePress()), this, SLOT(onKeyEscapePress()), Qt::QueuedConnection);

    // 依赖fcitx：当前可写状态
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusIn, this, &WordWizard::onFocusIn);
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusOut, this, &WordWizard::onFocusOut);
}

void WordWizard::onGlobalMousePress(int x, int y)
{
#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        m_isMousePress = true;

        if (m_selectwid->isWidgetVisible())
            if (m_selectwid->isMouseInside())
                return;
            else {
                m_selectwid->hide();
                m_selectwid->setWidgetVisible(false);
            }

        m_selectclip->clearClipText();
        m_isClose = false;
        return;
    }
#endif
    m_isMousePress = true;
    m_mouseClickX = x / m_scaleFactor;
    m_mouseClickY = y / m_scaleFactor;

    m_cursorPos = QPoint(m_mouseClickX, m_mouseClickY);
    getScreenRect();
    adjustMulScreenPos(m_cursorPos);

    m_mouseClickX = m_cursorPos.x();
    m_mouseClickY = m_cursorPos.y();

    QRect geoRect = m_selectwid->geometry();
    if ((geoRect.contains(m_cursorPos) || m_selectwid->isMouseInside()) && m_selectwid->isVisible()) {
        return;
    }

    geoRect = m_inputWindow->geometry();
    if ((geoRect.contains(m_cursorPos) || m_inputWindow->isMouseInside()) && m_inputWindow->isVisible()) {
        return;
    }

    if (m_selectwid->isVisible())
        m_selectwid->hide();

    if (m_inputWindow->isVisible())
        m_inputWindow->hide();

    m_selectclip->clearClipText();
    m_isClose = false;
    return;
}

void WordWizard::onGlobalMouseRelease(int x, int y)
{
#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        //剪贴板先触发，再触发释放（立即释放）
        // !m_isMouseRelease防止滚轮连续触发多次鼠标释放事件
        if (m_timer->isActive() && !m_isMouseRelease) {
            m_timer->stop();
            m_isMouseRelease = true;
            onShowScribeWord();
            return;
        }
        m_timer->start();

        m_isMouseRelease = true;

        if(!m_selectclip->isScribeWordsVisible() || m_selectwid->isWidgetVisible())
            return;

        //1. 剪贴板先触发，再触发释放（延迟释放）2.双击选中
        onShowScribeWord();
        return;
    }
#endif
    m_mouseReleaseX = x / m_scaleFactor;
    m_mouseReleaseY = y / m_scaleFactor;

    m_cursorPos = QPoint(m_mouseReleaseX, m_mouseReleaseY);
    getScreenRect();
    adjustMulScreenPos(m_cursorPos);

    m_mouseReleaseX = m_cursorPos.x();
    m_mouseReleaseY = m_cursorPos.y();

    //剪贴板先触发，再触发释放（立即释放）
    // !m_isMouseRelease防止滚轮连续触发多次鼠标释放事件
    if (m_timer->isActive() && !m_isMouseRelease) {
        m_timer->stop();
        m_isMouseRelease = true;
        onShowScribeWord();
        return;
    }
    m_timer->start();

    m_isMouseRelease = true;


    if(!m_selectclip->isScribeWordsVisible())
        return;

    QRect geoRect = m_selectwid->geometry();
    if (geoRect.contains(m_mouseClickX, m_mouseClickY) || m_selectwid->isVisible() || m_inputWindow->isVisible() )
        return;
        
    //1. 剪贴板先触发，再触发释放（延迟释放）2.双击选中
    onShowScribeWord();
}

void WordWizard::onKeyEscapePress()
{
    m_isClose = true;
    
    if (m_inputWindow->isVisible()) {
        m_inputWindow->close();
        return;
    }
    
    m_selectwid->close();
    emit m_selectwid->signalEscEvent();
}

void WordWizard::onShowScribeWord()
{
    // x11环境获取当前app信息
    m_curApp     = dynamic_cast<BaseMonitor *>(m_eventMonitor)->getCurApp();
    m_curPid     = dynamic_cast<BaseMonitor *>(m_eventMonitor)->getCurPid();

    QString selectedText = m_selectclip->getClipText();
    
    // 检查当前应用是否被禁用（快捷键不禁用）
    if (!m_isShortcut && isAppDisabled(m_curApp)) {
        qCInfo(logWordWizard) << "App disabled, not showing WizardWrapper:" << m_curApp;
        return;
    }
    if (!m_isShortcut && isProcessDisabled(m_curPid, m_curApp)) {
        qCInfo(logWordWizard) << "Process disabled, not showing WizardWrapper:" << m_curPid <<"("<<m_curApp<<")";
        return;
    }

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        bool isShow = true;
        if (m_stopShow)
            isShow = false;
        if (!m_isShortcut && m_isHidden)
            isShow = false;

        if (isShow) {
            if (m_isShortcut) {
                m_selectwid->switchToExpandedMode();
            } else {
                m_selectwid->switchToInitMode();
            }

            m_Point = QPoint(-1, -1);

            m_selectwid->showScribeWordsAtCursorPosition(m_screenRect, m_Point, m_isMouseRelease, m_isShortcut);
            m_isClose = false;

            m_isMousePress = false;
            m_isMouseRelease = false;
        }
        m_selectwid->setKnowledgeActionEnabled(selectedText.length() >= 10);
        return;
    }
#endif
    bool isShow = true;
    if (m_stopShow)
        isShow = false;
    if (!m_isShortcut && m_isHidden)
        isShow = false;
    if (m_selectwid->isVisible() || (m_isShortcut && !m_isClose && !m_isHidden))
        isShow = false;

    if (isShow) {
        if (m_isShortcut) {
            m_selectwid->switchToExpandedMode();
        } else {
            m_selectwid->switchToInitMode();
        }

        if (m_isShortcut)
            m_Point = QCursor::pos();
        else
            m_Point = QPoint(m_mouseReleaseX, m_mouseReleaseY + 16);

        m_selectwid->showScribeWordsAtCursorPosition(m_screenRect, m_Point, m_isMouseRelease, m_isShortcut);
        m_isClose = false;

        m_isMousePress = false;
        m_isMouseRelease = false;
    }
    m_selectwid->setKnowledgeActionEnabled(selectedText.length() >= 10);
}

bool WordWizard::launchUosBrowser(QString url) {
    bool ret = false;
    static QString mimetype = "x-scheme-handler/http";
    GList *list = g_app_info_get_all_for_type(mimetype.toStdString().c_str());
    // 遍历列表
    for (GList *l = list; l != nullptr; l = l->next) {
        // 注意：不需要手动释放 GAppInfo 对象，因为它们是由 g_app_info_get_all_for_type 管理的
        GAppInfo *appInfo = G_APP_INFO(l->data);
        const char *APP_PATH = g_app_info_get_executable(appInfo);
        qCInfo(logWordWizard) << "Find browser:" << APP_PATH;
        // /usr/bin/browser
        if (QString(APP_PATH).endsWith("/browser")) {
            GList *g_files = nullptr;
            GFile *f = g_file_new_for_uri(url.toStdString().c_str());
            g_files = g_list_append(g_files, f);
            GError *gError = nullptr;
            qCInfo(logWordWizard) << "Browser:" << url;
            gboolean ok = g_app_info_launch(appInfo, g_files, nullptr, &gError);
            if (gError) {
                qCWarning(logWordWizard) << "Error when trying to open desktop file with gio: " << gError->message;
                g_error_free(gError);
            }
            if (!ok) {
                qCWarning(logWordWizard) << "Failed to open desktop file with gio: g_app_info_launch returns false";
            } else {
                ret = true;
            }
            g_list_free_full(g_files, g_object_unref);
            break;
        }

        // 如果是玲珑环境，浏览器被玲珑接管，参数需要直接跟网址（如果玲珑环境安装了多个浏览器，会使用新安装的那个）
        // /usr/bin/ll-cli
        if (QString(APP_PATH).endsWith("/ll-cli")) {
            GList *g_files = nullptr;
            GFile *f = g_file_new_for_uri(url.toStdString().c_str());
            g_files = g_list_append(g_files, f);
            GError *gError = nullptr;
            qCInfo(logWordWizard) << QString("ll-cli %1").arg(QString("run org.deepin.browser -- browser ") + url);
            gboolean ok = g_app_info_launch(appInfo, g_files, nullptr, &gError);
            if (gError) {
                qCWarning(logWordWizard) << "Error when trying to open desktop file with gio: " << gError->message;
                g_error_free(gError);
            }
            if (!ok) {
                qCWarning(logWordWizard) << "Failed to open desktop file with gio: g_app_info_launch returns false";
            } else {
                ret = true;
            }
            g_list_free_full(g_files, g_object_unref);
            break;
        }
    }
    // 释放 GList 列表
    g_list_free_full(list, g_object_unref);

    return ret;
}

bool WordWizard::launchDefaultBrowser(QString url) {
    static QString mimetype = "x-scheme-handler/http";
    GAppInfo *defaultApp = g_app_info_get_default_for_type(mimetype.toLocal8Bit().constData(), FALSE);
    if (!defaultApp) {
        qCWarning(logWordWizard) << "No default browser found";
        return false;
    }

    bool ret = false;
    GList *g_files = nullptr;
    GFile *f = g_file_new_for_uri(url.toStdString().c_str());
    g_files = g_list_append(g_files, f);
    GError *gError = nullptr;
    qCInfo(logWordWizard) << "Browser:" << url;
    gboolean ok = g_app_info_launch(defaultApp, g_files, nullptr, &gError);
    if (gError) {
        qCWarning(logWordWizard) << "Error when trying to open desktop file with gio: " << gError->message;
        g_error_free(gError);
    }
    if (!ok) {
        qCWarning(logWordWizard) << "Failed to open desktop file with gio: g_app_info_launch returns false";
    } else {
        ret = true;
    }
    g_list_free_full(g_files, g_object_unref);
    g_object_unref(defaultApp);

    return ret;
}

bool WordWizard::onSearchBtnClicked(const QString &text)
{
    static const int MAX_TEXT_SIZE = 905;
    QString subtext = text;
    subtext.replace("\n", " ");
    if (subtext.size() > MAX_TEXT_SIZE) {
        subtext = subtext.mid(0, MAX_TEXT_SIZE);
    }

    const QString URL = "https://www.sou.com/?q=" + subtext + "&src=360tob_union_add";
    qCInfo(logWordWizard) << "default browser:" << URL;
    return launchDefaultBrowser(URL);
}

bool WordWizard::doAddToKnowledgeBase(const QString &text)
{
    KnowledgeBaseManager::AddFileResult addResult = KnowledgeBaseManager::getInstance().addTextAsFile(text);
    if (addResult.result == KnowledgeBaseManager::AddResult::Success)
        return true;

    qWarning() << "Add text to Knowledge Base failed. err: " << addResult.errorMessage;
    return false;
}

void WordWizard::onHiddenActionTriggered()
{
    onChangeHiddenStatus(false);
    emit signalHiddenwidget(true);
}

void WordWizard::onChangeXEventMonitorStatus(bool isopen)
{
    if (isopen) {
        m_eventMonitor->start();
        m_stopShow = true;
    } else {
        m_eventMonitor->terminate();
        m_selectwid->close();
        m_stopShow = true;
    }
}

void WordWizard::onFunctionTriggered(int wizardtype, const QPoint &cursorPos, bool isCustom)
{
    if (isCustom) {
        m_selectclip->clearClipText();
        emit sigToLaunchAiQuick(wizardtype, m_clipText, cursorPos, isCustom);
        m_selectwid->close();
        m_selectwid->setWidgetVisible(false);
    } else {
        if (wizardtype == WIZARD_TYPE_HIDDEN) {
            onHiddenActionTriggered();
            m_selectwid->close();
            m_inputWindow->close();
            m_selectwid->setWidgetVisible(false);
        } else if (wizardtype == WIZARD_TYPE_CUSTOM) {
            QString locateTitle(tr("UOS AI FollowAlong"));
            emit sigToLaunchMgmt(false, false, false, locateTitle);
            m_selectwid->close();
            m_inputWindow->close();
            m_selectwid->setWidgetVisible(false);
        } else if (wizardtype == WIZARD_TYPE_SEARCH) {
            if(onSearchBtnClicked(m_selectclip->getClipText())) {
               m_selectwid->close();
               m_selectwid->setWidgetVisible(false);
            }
        } else if (wizardtype == WIZARD_TYPE_KNOWLEDGE){
            m_selectwid->close();
            m_selectwid->setWidgetVisible(false);
            Q_EMIT sigToCloseAiQuick();
            ReportIns()->writeEvent(report::KnowledgeFunctionPoint("FollowAlong").assemblingData());
            if (doAddToKnowledgeBase(m_selectclip->getClipText())) {
                // 显示成功Toast
                m_selectwid->showToast(tr("Added"));
            } else {
                // 可选：显示错误Toast
                // m_selectwid->showToast(tr("Failed to add"));
            }
        } else {
            m_selectclip->clearClipText();
            emit sigToLaunchAiQuick(wizardtype, m_clipText, cursorPos, isCustom);
            m_selectwid->close();
            m_selectwid->setWidgetVisible(false);
        }
    }

    // tid:1001600002 event:followalong_function
    ReportIns()->writeEvent(report::FollowFunctionPoint(wizardtype).assemblingData());
}

void WordWizard::onSaveClipText()
{
    m_clipText = m_selectclip->getClipText();
}

void WordWizard::onShortcutPressed()
{
    m_isShortcut = true;
    onSaveClipText();
    if (!m_clipText.isEmpty())
        onShowScribeWord();
    else if (kIsFcitxWritable) {
        m_writerwid->showDialog();
    }
    m_isShortcut = false;
}

void WordWizard::onShortcutTranslate()
{
    onSaveClipText();
    if (!m_clipText.isEmpty())
        onFunctionTriggered(WIZARD_TYPE_TRANSLATE, QCursor::pos(), false);
}

void WordWizard::onCloseBtnClicked()
{
    m_isClose = true;
}

void WordWizard::onIconBtnClicked(QString text)
{
    qCInfo(logWordWizard) << "UOS AI Icon clicked, text:" << text;
    // 写作助手，点击Icon只打开主窗口
    // 划词助手，点击Icon打开主窗口并带入文本
    if (text.isEmpty()) {
        EAiExec()->showChatWindow();
    } else {
        emit ServerWrapper::instance()->sigAppendPrompt(text);
    }
}

bool WordWizard::queryHiddenStatus()
{
    return m_isHidden;
}

void WordWizard::onChangeHiddenStatus(bool isShow)
{
    qCInfo(logWordWizard) << "Changing hidden status to:" << isShow;
    DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_ISHIDDEN, !isShow);
    m_isHidden = !isShow;
    m_selectwid->isEnabledAction(isShow);
    m_inputWindow->isEnabledAction(isShow);
}

void WordWizard::onDisableInApp()
{
    qCInfo(logWordWizard) << "Disabling in app:" << m_curApp;
    if (!m_disabledApps.contains(m_curApp)) {
        m_disabledApps.push_front(m_curApp);
        DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_DISABLED_APPS, m_disabledApps);
    }

    m_selectclip->clearClipText();
    emit signalAddDisabledApp(m_curApp);
    m_selectwid->close();
    m_inputWindow->close();
    m_selectwid->setWidgetVisible(false);
}

void WordWizard::onDisableInProcess()
{
    qCInfo(logWordWizard) << "Disabling in process:" <<m_curPid << "("<<m_curApp<<")";
    m_disabledProcess[m_curPid] = m_curApp;
    m_selectclip->clearClipText();
    m_selectwid->close();
    m_inputWindow->close();
    m_selectwid->setWidgetVisible(false);
}

void WordWizard::checkDisabledProcesses()
{
    QMutableMapIterator<int, QString> it(m_disabledProcess);
    while (it.hasNext()) {
        it.next();
        int pid = it.key();
        QString expectedApp = it.value();
        
        // 检查进程是否存在且应用名匹配
        QString currentApp = dynamic_cast<BaseMonitor *>(m_eventMonitor)->getAppByPid(pid);
        if (currentApp.isEmpty() || currentApp != expectedApp) {
            qCInfo(logWordWizard) << "Removing invalid process entry:" << pid << "(" << expectedApp << ")";
            it.remove();
        }
    }
}

void WordWizard::getScreenRect()
{
    QList<QScreen *> screens = QGuiApplication::screens();

    for(QScreen *screen : screens) {
        qreal scaleFactor = screen->devicePixelRatio();
        QRect geometry = screen->geometry();
        QRect tempRect(geometry.x() / scaleFactor, geometry.y() / scaleFactor, geometry.width(), geometry.height());

        int top = tempRect.top() / scaleFactor;
        int m_cursorPosY = m_cursorPos.y() > top ? m_cursorPos.y() : top;
        if (m_cursorPosY - tempRect.bottom() > 0) {
            m_cursorPosY = tempRect.bottom() - (m_cursorPosY - tempRect.bottom());
        }
        if (tempRect.contains(m_cursorPos.x(), m_cursorPosY))
            m_screenRect = screen->geometry();
    }
}

void WordWizard::adjustMulScreenPos(QPoint &pos)
{

    qreal scaleFactor = QGuiApplication::primaryScreen()->devicePixelRatio();

    int x = m_screenRect.topLeft().x();
    int y = m_screenRect.topLeft().y();

    if (pos.x() > x / scaleFactor)
        pos.setX(pos.x() + x * (scaleFactor - 1) / scaleFactor);

    if (pos.y() > y / scaleFactor)
        pos.setX(pos.x() + y * (scaleFactor - 1) / scaleFactor);

}

bool WordWizard::isAppDisabled(const QString &appName)
{
    if (ESystemContext::isWayland() || appName.isEmpty())
        return false;
    return m_disabledApps.contains(appName);;
}

bool WordWizard::isProcessDisabled(int pid, const QString &appName)
{
    if (ESystemContext::isWayland() || pid == 0 || appName.isEmpty())
        return false;
    return m_disabledProcess[pid] == appName;
}

void WordWizard::updateDisabledApps(const QStringList &appList)
{
    if (m_disabledApps != appList) {
        m_disabledApps = appList;
        DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_DISABLED_APPS, m_disabledApps);
    }
}

QString WordWizard::getDefaultSkillName(int defaultFunctionType)
{
    switch (defaultFunctionType) {
    case WordWizard::WIZARD_TYPE_SEARCH:
        return QObject::tr("Search");
    case WordWizard::WIZARD_TYPE_EXPLAIN:
        return QObject::tr("Explain");
    case WordWizard::WIZARD_TYPE_SUMMARIZE:
        return QObject::tr("Summary");
    case WordWizard::WIZARD_TYPE_TRANSLATE:
        return QObject::tr("Translate");
    case WordWizard::WIZARD_TYPE_RENEW:
        return QObject::tr("Continue Writing");
    case WordWizard::WIZARD_TYPE_EXTEND:
        return QObject::tr("Expand");
    case WordWizard::WIZARD_TYPE_POLISH:
        return QObject::tr("Polish");
    case WordWizard::WIZARD_TYPE_CORRECT:
        return QObject::tr("Correct");
    case WordWizard::WIZARD_TYPE_KNOWLEDGE:
        return QObject::tr("Add to the AI knowledge base");
    default:
        return QString();
    }
}

QIcon WordWizard::getDefaultSkillIcon(int defaultFunctionType)
{
    switch (defaultFunctionType) {
    case WordWizard::WIZARD_TYPE_SEARCH:
        return QIcon::fromTheme("uos-ai-assistant_ai_search");
    case WordWizard::WIZARD_TYPE_EXPLAIN:
        return QIcon::fromTheme("uos-ai-assistant_explain");
    case WordWizard::WIZARD_TYPE_SUMMARIZE:
        return QIcon::fromTheme("uos-ai-assistant_summarize");
    case WordWizard::WIZARD_TYPE_TRANSLATE:
        return QIcon::fromTheme("uos-ai-assistant_translation");
    case WordWizard::WIZARD_TYPE_RENEW:
        return QIcon::fromTheme("uos-ai-assistant_renew");
    case WordWizard::WIZARD_TYPE_EXTEND:
        return QIcon::fromTheme("uos-ai-assistant_extend");
    case WordWizard::WIZARD_TYPE_POLISH:
        return QIcon::fromTheme("uos-ai-assistant_polish");
    case WordWizard::WIZARD_TYPE_CORRECT:
        return QIcon::fromTheme("uos-ai-assistant_correct");
    case WordWizard::WIZARD_TYPE_KNOWLEDGE:
        return QIcon::fromTheme("uos-ai-assistant_knowledge");
    default:
        return QIcon::fromTheme("uos-ai-assistant_custom");
    }
}

void WordWizard::onShowInputWindow(const QPoint &pos, const QRect &screenRect, int wizardWidth, int wizardHeight)
{
    // 延迟显示InputWindow，等待WizardWrapper收起动画完成
    QTimer::singleShot(300, this, [this, pos, screenRect, wizardWidth, wizardHeight]() {
        m_inputWindow->showAtWizardWrapperPosition(pos, screenRect, wizardWidth, wizardHeight);
    });
}
