#include "mgmtwindow.h"
#include "serverwrapper.h"
#include "dbwrapper.h"
#include "dbuscontrolcenterrequest.h"
#include "embeddingserver.h"
#include "localmodelserver.h"
#include "gui/chat/private/eaiexecutor.h"
#include "uosfreeaccounts.h"
#include "dconfigmanager.h"
#include "util.h"
#include "utils/apputils.h"
#include "private/useragreementdialog.h"
#include "private/modellistwidget.h"
#include "private/operatinglinewidget.h"
#include "private/welcomedialog.h"
#include "private/addmodeldialog.h"
#include "private/addprivatemodeldialog.h"
#include "private/themedlable.h"
#include "private/localmodellistwidget.h"
#include "private/echatwndmanager.h"
#include "private/knowledgebaselistwidget.h"
#include "private/getfreeaccountdialog.h"
#include "private/wordwizardwidget.h"
#include "private/aibarwidget.h"
#include "private/mcpserverwidget.h"
#include "private/skillserverwidget.h"
#include "private/privatemodellistwidget.h"
#include "private/chatbotwidget.h"

#include <DWidgetUtil>
#include <DLabel>
#include <DBackgroundGroup>
#include <DFrame>
#include <DGuiApplicationHelper>
#include <DTitlebar>
#include <DDialog>
#include <DFloatingMessage>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QApplication>
#include <QTimer>
#include <QLoggingCategory>
#include <QGraphicsDropShadowEffect>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

Q_DECLARE_METATYPE(QMargins)
const QVariant VListViewItemMargin = QVariant::fromValue(QMargins(15, 0, 5, 0));

static constexpr char ShowInfo_Ok[] = ":/assets/images/ok_info.svg";

MgmtWindow::MgmtWindow(DWidget *parent)
    : DMainWindow(parent)
{
//    EWndManager()->registeWindow(this);

    if (Util::checkLanguage()) {
        setFixedSize(800, 568);
    } else {
        setFixedSize(860, 568);
    }
    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);              // 禁止窗口最大化和最小化按钮
    setWindowFlag(Qt::Dialog);                                       // 取消在底栏显示
    setWindowModality(Qt::NonModal);

    m_pAddDlg = new AddModelDialog();
    m_pAddPrivateDlg = new AddPrivateModelDialog();
    m_pGetFreeAccountDialog = new GetFreeAccountDialog();

    initUI();
    initConnect();
    // 显示在显示器中心
    Dtk::Widget::moveToCenter(this);
}

MgmtWindow::~MgmtWindow()
{
    if (m_pAddDlg) {
        m_pAddDlg->deleteLater();
    }
    if (m_pAddPrivateDlg) {
        m_pAddPrivateDlg->deleteLater();
    }
    if (m_pGetFreeAccountDialog) {
        m_pGetFreeAccountDialog->deleteLater();
    }
}

void MgmtWindow::initUI()
{
    DTitlebar *mainTitlebar = titlebar();
    mainTitlebar->setQuitMenuVisible(false);

    for (auto action : mainTitlebar->menu()->actions()) {
        if (qApp->translate("TitleBarMenu", "Feedback") == action->text()) {
            action->setVisible(false);
            break;
        }
    }
    // 隐藏标题栏中的menu
    titlebar()->setMenuVisible(false);

    DWidget *mainWidget = new DWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setContentsMargins(10, 10, 10, 10);

    DFrame *frame = new DFrame(this);
    frame->setFixedWidth(600);
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frame->setLineWidth(0);

    DWidget *scrollWidget = new DWidget(this);
    scrollWidget->setContentsMargins(0, 10, 0, 10);
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);

    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->setWidget(scrollWidget);
    m_pScrollArea->setFrameShape(QFrame::NoFrame);
    m_pScrollArea->setWindowFlags(Qt::FramelessWindowHint);
    m_pScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    m_pScrollArea->setContentsMargins(0, 0, 0, 0);
    m_pScrollArea->setLineWidth(0);
    m_pScrollArea->setAttribute(Qt::WA_TranslucentBackground);
    m_pScrollArea->installEventFilter(this);

    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(30);

    scrollLayout->addWidget(initModelConfigWidget(), 0, Qt::AlignCenter);

#ifdef ENABLE_MCP
    scrollLayout->addWidget(initMcpServerWidget(), 0, Qt::AlignCenter);
    scrollLayout->addWidget(initSkillWidget(), 0, Qt::AlignCenter);
#endif

    scrollLayout->addWidget(initWordWizardWidget(), 0, Qt::AlignLeft);

#ifdef ENABLE_ASSISTANT
    m_pKnowledgeBaseListWidget = initKnowledgeBaseWidget();
    scrollLayout->addWidget(m_pKnowledgeBaseListWidget, 0, Qt::AlignCenter);
#endif

#ifdef ENABLE_CHATBOT
    scrollLayout->addWidget(initChatBotWidget(), 0, Qt::AlignCenter);
#endif
    scrollLayout->addWidget(initProxyWidget(), 0, Qt::AlignCenter);
    scrollLayout->addWidget(initAgreementWidget(), 0, Qt::AlignCenter);

#ifdef ENABLE_AI_BAR
    scrollLayout->addWidget(initAiBarWidget(), 0, Qt::AlignCenter);
#endif
    scrollLayout->addStretch();

    frameLayout->addWidget(m_pScrollArea);
    rightLayout->addWidget(frame, 0, Qt::AlignCenter);

    //添加导航栏
    m_pNavigationWidget = new Navigation;
    m_pNavigationWidget->updateNavigationTitles(widgetList);

    mainLayout->addWidget(m_pNavigationWidget);
    mainLayout->addLayout(rightLayout);

    setCentralWidget(mainWidget);

    onThemeTypeChanged();
}

void MgmtWindow::initConnect()
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MgmtWindow::onThemeTypeChanged);
    connect(WelcomeDialog::instance(), &WelcomeDialog::signalAppendModel, m_pModelListWidget, &ModelListWidget::onAppendModel);
    connect(WelcomeDialog::instance(), &WelcomeDialog::accepted, [this] {
        if (!WelcomeDialog::instance()->isFreeAccount() && !WelcomeDialog::instance()->isOnlyUseAgreement()) {
            showEx(false,false);  //点击新手流程弹窗的【添加模型】，不再弹出添加模型弹窗，而是跳转到设置的首页。
        }
    });

    connect(m_pModelListWidget, &ModelListWidget::signalGetFreeAccountClicked, this, &MgmtWindow::onShowGetFreeAccountDialog);
    connect(m_pGetFreeAccountDialog, &GetFreeAccountDialog::signalAppendModel, m_pModelListWidget, &ModelListWidget::onAppendModel);
    connect(m_pGetFreeAccountDialog, &GetFreeAccountDialog::signalActivityEnd, this, &MgmtWindow::onHiddenGetFreeAccountBtn);
    connect(m_pAddDlg, &AddModelDialog::accepted, [this] {
        m_pModelListWidget->onAppendModel(m_pAddDlg->getModelData());

        if (m_pLocalModelListWidget) m_pLocalModelListWidget->updateLocalModelList();
        this->activateWindow();
        this->show();

        showFloatingMessage(tr("Successfully connected"));

        if (m_isFromAiQuick) {
            EAiExec()->setUosAiLLMAccountId(m_pAddDlg->getModelData().id);
        }
    });

    connect(m_pAddPrivateDlg, &AddModelDialog::accepted, this, &MgmtWindow::onAddPrivateModel);
    connect(m_pScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MgmtWindow::onscrollAreaValueChanged);
    connect(this, &MgmtWindow::scrollToGroup, m_pNavigationWidget, [ = ](const QString & key) {
        m_pNavigationWidget->blockSignals(true);
        m_pNavigationWidget->onSelectGroup(key);
        m_pNavigationWidget->blockSignals(false);
    });

    connect(m_pNavigationWidget, &Navigation::selectedGroup, this, &MgmtWindow::onNavigationSelected);

    if (m_pKnowledgeBaseListWidget)
        connect(m_pKnowledgeBaseListWidget, &KnowledgeBaseListWidget::sigGenPersonalFAQ, this, &MgmtWindow::sigGenPersonalFAQ);

    if (m_pWordWizardWidget) {
        connect(m_pWordWizardWidget, &WordWizardWidget::signalChangeHiddenStatus, this, [ = ](bool isShow){
            onWordWizardHiddenStatus(!isShow);
            emit signalWordWizardStatusChanged(isShow);
        });
        connect(m_pWordWizardWidget, &WordWizardWidget::disabledAppsUpdateRequested, this, &MgmtWindow::signalDisabledAppsUpdated);
        connect(m_pWordWizardWidget, &WordWizardWidget::skillAddedSuccessfully, this, &MgmtWindow::showFloatingMessage);
    }

    if (m_pAiBarWidget)
        connect(m_pAiBarWidget, &AiBarWidget::signalChangeDragStatus, this, [ = ](bool enable){
            DConfigManager::instance()->setValue(AIBAR_GROUP, AIBAR_ENABLEFILEDRAG, enable);
        });

    if (m_pLocalModelListWidget)
        connect(m_pLocalModelListWidget, &LocalModelListWidget::sigRedPointVisible, this, [ = ](bool isUpdate){
            if (!this->isVisible())
                emit sigSetRedPointVisible(isUpdate);
        });
}

bool MgmtWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pScrollArea && event->type() == QEvent::Resize) {
        m_pScrollArea->widget()->setFixedWidth(m_pScrollArea->width());
    }
    return DMainWindow::eventFilter(watched, event);
}

void MgmtWindow::closeEvent(QCloseEvent *event)
{
    qCDebug(logAIGUI) << "MgmtWindow closing";
    if (m_pModelListWidget)
        m_pModelListWidget->resetEditButton();

    if (m_pPrivateModelListWidget)
        m_pPrivateModelListWidget->resetEditButton();

    if (m_pKnowledgeBaseListWidget)
        m_pKnowledgeBaseListWidget->resetEditButton();

    if (m_pLocalModelListWidget) {
        m_pLocalModelListWidget->updateLocalModelList();
        m_pLocalModelListWidget->clearRedPoint();
    }

    emit signalCloseWindow();

    EmbeddingServer::getInstance().saveAllIndex();
    qCInfo(logAIGUI) << "MgmtWindow closed, all indexes saved";

    DMainWindow::closeEvent(event);
}

void MgmtWindow::onThemeTypeChanged()
{
    for (auto item : m_widgets) {
        DPalette pl = item->palette();
        pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
        item->setPalette(pl);
    }
}

ModelListWidget *MgmtWindow::initModelListWidget()
{
    m_pModelListWidget = new ModelListWidget(this);
    m_pModelListWidget->setProperty("title", m_pModelListWidget->getTitleName());
    m_pModelListWidget->setProperty("level", 2);
    titles.insert(m_pModelListWidget->getTitleName(),m_pModelListWidget);
    widgetList.push_back(m_pModelListWidget);
    m_pModelListWidget->setModelList(DbWrapper::localDbWrapper().queryLlmList());
    connect(m_pModelListWidget, &ModelListWidget::signalAddModel, this, &MgmtWindow::onAddModel);
    return m_pModelListWidget;
}

PrivateModelListWidget *MgmtWindow::initPrivateModelListWidget()
{
    m_pPrivateModelListWidget = new PrivateModelListWidget(this);
    m_pPrivateModelListWidget->setProperty("title", m_pPrivateModelListWidget->getTitleName());
    m_pPrivateModelListWidget->setProperty("level", 2);
    titles.insert(m_pPrivateModelListWidget->getTitleName(),m_pPrivateModelListWidget);
    widgetList.push_back(m_pPrivateModelListWidget);
    m_pPrivateModelListWidget->setModelList(DbWrapper::localDbWrapper().queryLlmList());
    connect(m_pPrivateModelListWidget, &PrivateModelListWidget::signalAddModel, this, &MgmtWindow::showAddPrivateModel);
    return m_pPrivateModelListWidget;
}

LocalModelListWidget *MgmtWindow::initLocalModelListWidget()
{
    m_pLocalModelListWidget = new LocalModelListWidget(this);
    m_pLocalModelListWidget->setProperty("title", m_pLocalModelListWidget->getTitleName());
    m_pLocalModelListWidget->setProperty("level", 2);
    titles.insert(m_pLocalModelListWidget->getTitleName(),m_pLocalModelListWidget);
    widgetList.push_back(m_pLocalModelListWidget);
    return m_pLocalModelListWidget;
}

void MgmtWindow::onAddModel()
{
    qCInfo(logAIGUI) << "Opening add model dialog";
    m_pAddDlg->resetDialog();
    m_pAddDlg->show();
    m_pAddDlg->adjustSize();
    m_pAddDlg->activateWindow();
}

void MgmtWindow::showAddPrivateModel()
{
    qCInfo(logAIGUI) << "Opening add private model dialog";
    m_pAddPrivateDlg->resetDialog();
    m_pAddPrivateDlg->show();
    m_pAddPrivateDlg->adjustSize();
    m_pAddPrivateDlg->activateWindow();
}

void MgmtWindow::onAddPrivateModel()
{
    qCInfo(logAIGUI) << "Adding new private model";
    m_pPrivateModelListWidget->onAppendModel(m_pAddPrivateDlg->getModelData());

    if (m_pLocalModelListWidget) m_pLocalModelListWidget->updateLocalModelList();
    this->activateWindow();
    this->show();

    showFloatingMessage(tr("Successfully connected"));

    if (m_isFromAiQuick) {
        qCDebug(logAIGUI) << "Setting UOS AI LLM account ID from quick access";
        EAiExec()->setUosAiLLMAccountId(m_pAddDlg->getModelData().id);
    }
}

DWidget *MgmtWindow::initAgreementWidget()
{
    DWidget *widget = new DWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(10);

    ThemedLable *label = new ThemedLable(tr("User Agreement"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Bold);
    widget->setProperty("title", label->text());
    widget->setProperty("level", 1);
    titles.insert(label->text(),widget);
    widgetList.push_back(widget);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    OperatingLineWidget *oper = new OperatingLineWidget(this);
    oper->setName(tr("UOS AI User Agreement"));
    oper->setEditText(tr("Read and agreed"));

    connect(oper, &OperatingLineWidget::signalNotDeleteButtonClicked, this, [this]() {
        UserAgreementDialog dlg;
        dlg.exec();
    });

    hLayout->addWidget(oper);
    DBackgroundGroup *bgGroup = new DBackgroundGroup(hLayout, this);
    bgGroup->setContentsMargins(0, 0, 0, 0);
    bgGroup->setFixedHeight(36);
    m_widgets.insert(bgGroup);

    vLayout->addWidget(label, 0, Qt::AlignLeft);
    vLayout->addWidget(bgGroup);

    return widget;
}

DWidget *MgmtWindow::initProxyWidget()
{
    DWidget *widget = new DWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(10);

    ThemedLable *label = new ThemedLable(tr("Proxy Settings"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Bold);
    widget->setProperty("title", label->text());
    widget->setProperty("level", 1);
    titles.insert(label->text(),widget);
    widgetList.push_back(widget);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    OperatingLineWidget *oper = new OperatingLineWidget(this);
    oper->setName(tr("Proxy Settings"));
    oper->setEditText(tr("Go to settings"));

    connect(oper, &OperatingLineWidget::signalNotDeleteButtonClicked, this, []() {
        DbusControlCenterRequest dbus;
#ifdef COMPILE_ON_V23
        dbus.showPage("network/systemProxy");
#else
        dbus.showPage("network", "System Proxy");
#endif
    });

    hLayout->addWidget(oper);
    DBackgroundGroup *bgGroup = new DBackgroundGroup(hLayout, this);
    bgGroup->setContentsMargins(0, 0, 0, 0);
    bgGroup->setFixedHeight(36);
    m_widgets.insert(bgGroup);

    vLayout->addWidget(label, 0, Qt::AlignLeft);
    vLayout->addWidget(bgGroup);

    return widget;
}

KnowledgeBaseListWidget *MgmtWindow::initKnowledgeBaseWidget()
{
    KnowledgeBaseListWidget *widget = new KnowledgeBaseListWidget(this);
    widget->setFixedWidth(560);
    widget->setProperty("title", widget->getTitleName());
    widget->setProperty("level", 1);
    titles.insert(widget->getTitleName(),widget);
    widgetList.push_back(widget);
//    widget->setKnowledgeBaseList();
//    connect(widget, &KnowledgeBaseListWidget::signalAddKnowledgeBase, this, &MgmtWindow::onAddKnowledgeBase);
    return widget;
}

WordWizardWidget *MgmtWindow::initWordWizardWidget()
{
    m_pWordWizardWidget = new WordWizardWidget(this);
    m_pWordWizardWidget->setProperty("title", m_pWordWizardWidget->getTitleName());
    m_pWordWizardWidget->setProperty("level", 1);
    titles.insert(m_pWordWizardWidget->getTitleName(),m_pWordWizardWidget);
    widgetList.push_back(m_pWordWizardWidget);
    return m_pWordWizardWidget;
}

AiBarWidget *MgmtWindow::initAiBarWidget()
{
    m_pAiBarWidget = new AiBarWidget(this);
    m_pAiBarWidget->setFixedWidth(560);
    m_pAiBarWidget->setProperty("title", m_pAiBarWidget->getTitleName());
    m_pAiBarWidget->setProperty("level", 1);
    titles.insert(m_pAiBarWidget->getTitleName(),m_pAiBarWidget);
    widgetList.push_back(m_pAiBarWidget);
    return m_pAiBarWidget;
}

DWidget *MgmtWindow::initMcpServerWidget()
{
    m_pMcpServerWidget = new McpServerWidget(this);

    m_pMcpServerWidget->setFixedWidth(560);
    m_pMcpServerWidget->setProperty("title", m_pMcpServerWidget->getTitleName());
    m_pMcpServerWidget->setProperty("level", 1);
    titles.insert(m_pMcpServerWidget->getTitleName(), m_pMcpServerWidget);
    widgetList.push_back(m_pMcpServerWidget);
    connect(this, &MgmtWindow::sigThirdPartyMcpAgree, m_pMcpServerWidget, &McpServerWidget::sigThirdPartyMcpAgree);
    return m_pMcpServerWidget;
}

DWidget *MgmtWindow::initSkillWidget()
{
    m_pSkillServerWidget = new SkillServerWidget(this);

    m_pSkillServerWidget->setFixedWidth(560);
    m_pSkillServerWidget->setProperty("title", m_pSkillServerWidget->getTitleName());
    m_pSkillServerWidget->setProperty("level", 1);
    titles.insert(m_pSkillServerWidget->getTitleName(), m_pSkillServerWidget);
    widgetList.push_back(m_pSkillServerWidget);
    connect(this, &MgmtWindow::sigThirdPartyMcpAgree, m_pSkillServerWidget, &SkillServerWidget::sigThirdPartyMcpAgree);
    connect(m_pSkillServerWidget, &SkillServerWidget::sigNavigateToMcpServerPage, this, [this]() {
        if (m_pNavigationWidget) {
            m_pNavigationWidget->onSelectGroup(m_pMcpServerWidget->getTitleName());
        }
    });
    if (m_pMcpServerWidget) {
        m_pSkillServerWidget->changeInstallStatus(m_pMcpServerWidget->getIsInstalled());
        connect(m_pMcpServerWidget, &McpServerWidget::sigAgentInstallChanged, m_pSkillServerWidget, &SkillServerWidget::changeInstallStatus);
    }
    return m_pSkillServerWidget;
}

DWidget *MgmtWindow::initModelConfigWidget()
{
    DWidget *modelConfigWidget = new DWidget(this);
    modelConfigWidget->setFixedWidth(560);
    modelConfigWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *modelConfigLayout = new QVBoxLayout(modelConfigWidget);
    modelConfigLayout->setContentsMargins(0, 0, 0, 0);

    ThemedLable *label = new ThemedLable(tr("Model Configuration"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Bold);
    modelConfigWidget->setProperty("title", label->text());
    modelConfigWidget->setProperty("level", 1);
    titles.insert(label->text(),modelConfigWidget);
    widgetList.push_back(modelConfigWidget);

    modelConfigLayout->addWidget(label);
    modelConfigLayout->addWidget(initModelListWidget());
    modelConfigLayout->addSpacing(20);

#ifdef ENABLE_ASSISTANT
    modelConfigLayout->addWidget(initLocalModelListWidget());
    if (!m_pLocalModelListWidget->property("title").toString().isEmpty())
        modelConfigLayout->addSpacing(20);
#endif

    modelConfigLayout->addWidget(initPrivateModelListWidget());
    return modelConfigWidget;
}

void MgmtWindow::showEx(bool showAddllmPage, bool onlyUseAgreement, bool isFromAiQuick, const QString & locateTitle)
{
    m_isFromAiQuick = isFromAiQuick;

    //已经存在模态框，激活模态框继续操作
    auto addDlg = this->findChild<QDialog *>();
    if (addDlg && addDlg->isVisible()) {
        addDlg->showNormal();
        addDlg->activateWindow();
        return;
    }

    if (m_pLocalModelListWidget)
        m_pLocalModelListWidget->updateLocalModelList();
    if (m_pModelListWidget)
        m_pModelListWidget->checkActivityExists();
    if (m_pMcpServerWidget)
        m_pMcpServerWidget->updateStatus();
    if (m_pSkillServerWidget)
        m_pSkillServerWidget->updateStatus();
    if (m_pWordWizardWidget)
        m_pWordWizardWidget->updateHiddenStatus(m_bIsWordWizardHidden);
    if (m_pAiBarWidget)
        m_pAiBarWidget->updateDragStatus(DConfigManager::instance()->value(AIBAR_GROUP, AIBAR_ENABLEFILEDRAG).toBool());
    if (m_pKnowledgeBaseListWidget)
        m_pKnowledgeBaseListWidget->onRefresh();

    loadDisabledApps();

    this->activateWindow();
    this->show();
    m_pNavigationWidget->onSelectGroup(locateTitle);
    onNavigationSelected(locateTitle);
    if (showAddllmPage) {
        onAddModel();
    }
}

void MgmtWindow::checkUpdateStatus()
{
    if (m_pLocalModelListWidget)
        m_pLocalModelListWidget->updateLocalModelList();
}

void MgmtWindow::showGetFreeAccountDlg()
{
    onShowGetFreeAccountDialog();
}

void MgmtWindow::onShowGetFreeAccountDialog()
{
    qCDebug(logAIGUI) << "Showing get free account dialog";
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));

    UosFreeAccount freeAccount;
    int status;
    QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, UOS_FREE, freeAccount, status);

    if (1 == status) {
        qCWarning(logAIGUI) << "Free account activity has ended";
        dlg.setMessage(tr("The free account activity ends."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
        dlg.exec();
        onHiddenGetFreeAccountBtn();
        return;
    }
    if (DDialog::Accepted == m_pGetFreeAccountDialog->exec()) {
        qCInfo(logAIGUI) << "User accepted free account dialog";
        m_pGetFreeAccountDialog->onGetFreeAccount();
    }
}

void MgmtWindow::onscrollAreaValueChanged(int value)
{
    QList<DWidget *> visableSortTitles;
    for (auto idx = 0; idx < widgetList.length(); ++idx) {
        auto title = widgetList[idx];
        if (title->isVisible()) {
            visableSortTitles.push_back(title);
        }
    }
    if (visableSortTitles.empty())
        return;

    auto currentTitle = visableSortTitles.first();
    auto viewHeight = m_pScrollArea->height();

    QList<DWidget *> visableTitleList;

    for (auto idx = 0; idx < visableSortTitles.length(); ++idx) {
        auto title = visableSortTitles[idx];
        if (title->y() <= value) {
            if (idx < visableSortTitles.length() - 1) {
                auto nextTitle = visableSortTitles[idx + 1];
                if (nextTitle->y() >= value) {
                    visableTitleList.push_back(title);
                }
            }
        } else if (title->y() < (value + viewHeight)) {
            visableTitleList.push_back(title);
        }
    }
    if (!visableTitleList.isEmpty()) {
        auto lastTitle = visableSortTitles.last();
        // 最后一节内容所占超过300px高度，直接将标题设置为最后一节
        if (value + viewHeight - 300 >= lastTitle->y()) {
            currentTitle = visableTitleList.last();
        } else {
            currentTitle = visableTitleList.first();
        }
    }
    //滚动条到底
    if (value == m_pScrollArea->verticalScrollBar()->maximum())
        currentTitle = visableTitleList.last();

    if (value >= visableSortTitles.last()->y())
        currentTitle = visableSortTitles.last();
    if (value <= visableSortTitles.first()->y())
        currentTitle = visableSortTitles.first();

    if (currentTitle) {
        emit scrollToGroup(currentTitle->property("title").toString());
    }
}

void MgmtWindow::onNavigationSelected(const QString & key)
{
    int scrollBarValue = 0;//前端打开设置页面，默认在顶部
    if (!key.isNull()) {
        if (!titles.contains(key))
            return;
        auto title = titles.value(key);
        scrollBarValue = title->y();
    }
    this->blockSignals(true);
    m_pScrollArea->verticalScrollBar()->setValue(scrollBarValue);
    this->blockSignals(false);
}

void MgmtWindow::onWordWizardHiddenStatus(bool isHidden)
{
    m_bIsWordWizardHidden = !isHidden;
}

void MgmtWindow::onHiddenGetFreeAccountBtn()
{
    m_pModelListWidget->hiddenGetFreeAccountButton();
}

void MgmtWindow::onAddKnowledgeBase(const QStringList & filePath)
{
    if (filePath.isEmpty()) {
        qCWarning(logAIGUI) << "Attempted to add knowledge base with empty file path";
        return;
    }

    if (m_pKnowledgeBaseListWidget == nullptr) {
        qCWarning(logAIGUI) << "Knowledge base widget is null";
        return;
    }

    if (m_pKnowledgeBaseListWidget->checkEmbeddingPluginsStatus()) {
        qCInfo(logAIGUI) << "Adding knowledge base files:" << filePath;
        m_pKnowledgeBaseListWidget->addKnowledgeBaseFile(filePath);
    }
    else {
        qCWarning(logAIGUI) << "Vectorization model plugin not installed";
        DDialog dialog("",tr("Adding to the knowledge base requires installing the vectorization model plugin. Please go to the app store to download and install."), this);
        dialog.setFixedWidth(380);
        dialog.setIcon(QIcon(":assets/images/warning.svg"));
        auto labelList = dialog.findChildren<QLabel *>();
        for (auto messageLabel : labelList) {
            if ("MessageLabel" == messageLabel->objectName())
                messageLabel->setFixedWidth(dialog.width() - 20);
        }
        dialog.addButton(tr("Do not install", "button"), true, DDialog::ButtonNormal);
        dialog.addButton(tr("Install immediately", "button"), true, DDialog::ButtonRecommend);
        if (DDialog::Accepted == dialog.exec()) {
            qCInfo(logAIGUI) << "User chose to install vectorization plugin";
            LocalModelServer::getInstance().openInstallWidgetOnTimer(PLUGINSNAME);
        }
    }
}

void MgmtWindow::onAddDisabledApp(const QString &appName)
{
    if (m_pWordWizardWidget) {
        DisableAppWidget *disableWidget = m_pWordWizardWidget->getDisableAppWidget();
        if (disableWidget) {
            disableWidget->addApp(appName);
            disableWidget->updateLayout();//减少多余渲染
        }
    }
}

void MgmtWindow::loadDisabledApps()
{
    if (!m_pWordWizardWidget)
        return;

    // 从 DConfig 获取禁用应用列表
    QVariant disabledAppsVar = DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_DISABLED_APPS);
    if (disabledAppsVar.isValid()) {
        QStringList disabledApps = disabledAppsVar.toStringList();

        DisableAppWidget *disableWidget = m_pWordWizardWidget->getDisableAppWidget();
        if (disableWidget) {
            disableWidget->clearApps();
            if (disabledApps.isEmpty())
                return;

            for (const QString &appName : disabledApps) {
                disableWidget->addApp(appName);
            }
            disableWidget->updateLayout();//减少多余渲染
            emit signalDisabledAppsUpdated(disabledApps);
        }
    }
}

DWidget *MgmtWindow::initChatBotWidget()
{
    m_pChatBotWidget = new ChatBotWidget(this);
    m_pChatBotWidget->setFixedWidth(560);
    m_pChatBotWidget->setProperty("title", m_pChatBotWidget->getTitleName());
    m_pChatBotWidget->setProperty("level", 1);
    titles.insert(m_pChatBotWidget->getTitleName(), m_pChatBotWidget);
    widgetList.push_back(m_pChatBotWidget);
    return m_pChatBotWidget;
}

void MgmtWindow::showFloatingMessage(const QString &message)
{
    DFloatingMessage *floatingMessage = new DFloatingMessage(DFloatingMessage::TransientType, this);
    floatingMessage->setMessage(message);
    floatingMessage->setIcon(QIcon(ShowInfo_Ok));
    floatingMessage->setAttribute(Qt::WA_DeleteOnClose);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(floatingMessage);
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(2);
    shadowEffect->setYOffset(2);
    shadowEffect->setColor(QColor(0, 0, 0, int(255 * 0.2))); // (不透明度约20%)
    floatingMessage->setGraphicsEffect(shadowEffect);

    QRect geometry(QPoint(0, 0), floatingMessage->sizeHint());
    geometry.moveCenter(rect().center());
    geometry.moveBottom(rect().bottom() - 5);
    floatingMessage->setGeometry(geometry);

    floatingMessage->show();
}
