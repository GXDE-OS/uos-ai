#include "mgmtwindow.h"
#include "serverwrapper.h"
#include "dbwrapper.h"
#include "dbuscontrolcenterrequest.h"

#include "private/useragreementdialog.h"
#include "private/modellistwidget.h"
#include "private/operatinglinewidget.h"
#include "private/welcomedialog.h"
#include "private/addmodeldialog.h"
#include "private/themedlable.h"
#include "private/localmodellistwidget.h"
#include "private/echatwndmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QApplication>

#include <DWidgetUtil>
#include <DLabel>
#include <DBackgroundGroup>
#include <DFrame>
#include <DGuiApplicationHelper>
#include <DTitlebar>
#include <DDialog>

Q_DECLARE_METATYPE(QMargins)
const QVariant VListViewItemMargin = QVariant::fromValue(QMargins(15, 0, 5, 0));

static constexpr char ShowInfo_Ok[] = ":/assets/images/ok_info.svg";

MgmtWindow::MgmtWindow(DWidget *parent)
    : DMainWindow(parent)
{
    EWndManager()->registeWindow(this);

    setFixedSize(680, 616);
    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);              // 禁止窗口最大化和最小化按钮
    setWindowFlag(Qt::Dialog);                                       // 取消在底栏显示
    setWindowModality(Qt::ApplicationModal);

    m_pWelcomeDlg = new WelcomeDialog();
    m_pAddDlg = new AddModelDialog(this);

    initUI();
    initConnect();
    // 显示在显示器中心
    Dtk::Widget::moveToCenter(this);
}

MgmtWindow::~MgmtWindow()
{
    if (m_pWelcomeDlg)
        m_pWelcomeDlg->deleteLater();
}

void MgmtWindow::initUI()
{
    DTitlebar *mainTitlebar = titlebar();
//    mainTitlebar->setIcon(QIcon::fromTheme(QApplication::instance()->applicationName()));
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
    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(20, 10, 20, 10);
    frame->setLineWidth(0);

    DWidget *scrollWidget = new DWidget(this);
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

    m_pModelListWidget = initModelListWidget();

    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(30);
    scrollLayout->addWidget(m_pModelListWidget);
    scrollLayout->addWidget(initLocalModelListWidget());
    scrollLayout->addWidget(initProxyWidget());
    scrollLayout->addWidget(initAgreementWidget());
    scrollLayout->addStretch();

    frameLayout->addWidget(m_pScrollArea);
    rightLayout->addWidget(frame);

    mainLayout->addLayout(rightLayout);

    onThemeTypeChanged();
    setCentralWidget(mainWidget);
}

void MgmtWindow::initConnect()
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MgmtWindow::onThemeTypeChanged);
    connect(m_pWelcomeDlg, &WelcomeDialog::signalAppendModel, m_pModelListWidget, &ModelListWidget::onAppendModel);
    connect(m_pWelcomeDlg, &WelcomeDialog::accepted, [this] {
        if (!m_pWelcomeDlg->isFreeAccount())
        {
            showAddModelDialog(true);
        } else
        {
            DbWrapper::localDbWrapper().updateAICopilot(true);
            DbWrapper::localDbWrapper().updateUserExpState(m_pWelcomeDlg->getUserExpState() == Qt::Unchecked ? -1 : 1);
            ServerWrapper::instance()->updateUserExpState(m_pWelcomeDlg->getUserExpState() == Qt::Unchecked ? -1 : 1);
        }
    });

    connect(m_pAddDlg, &AddModelDialog::accepted, [this] {
        m_pModelListWidget->onAppendModel(m_pAddDlg->getModelData());

        if (m_pLocalModelListWidget) m_pLocalModelListWidget->updateLocalModelList();
        this->activateWindow();
        this->show();
        DFloatingMessage *message = new DFloatingMessage(DFloatingMessage::TransientType, this);
        message->setMessage(tr("Successfully connected"));
        message->setIcon(QIcon(ShowInfo_Ok));

        QRect geometry(QPoint(0, 0), message->sizeHint());
        geometry.moveCenter(rect().center());
        geometry.moveBottom(rect().bottom() - 5);
        message->setGeometry(geometry);

        message->show();

        if (m_bIsWelcomeAdd)
        {
            DbWrapper::localDbWrapper().updateAICopilot(true);
            DbWrapper::localDbWrapper().updateUserExpState(m_pWelcomeDlg->getUserExpState() == Qt::Unchecked ? -1 : 1);
            ServerWrapper::instance()->updateUserExpState(m_pWelcomeDlg->getUserExpState() == Qt::Unchecked ? -1 : 1);
        }
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
    if (m_pModelListWidget)
        m_pModelListWidget->resetEditButton();

    emit signalCloseWindow();

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
    ModelListWidget *widget = new ModelListWidget(this);
    widget->setModelList(DbWrapper::localDbWrapper().queryLlmList());
    connect(widget, &ModelListWidget::signalAddModel, this, &MgmtWindow::onAddModel);
    return widget;
}

LocalModelListWidget *MgmtWindow::initLocalModelListWidget()
{
    m_pLocalModelListWidget = new LocalModelListWidget(this);
    return m_pLocalModelListWidget;
}

void MgmtWindow::onAddModel()
{
    showAddModelDialog();
}

bool MgmtWindow::showAddModelDialog(bool isWelcomeAdd)
{
    m_bIsWelcomeAdd = isWelcomeAdd;
    m_pAddDlg->resetDialog();
    m_pAddDlg->show();
    m_pAddDlg->activateWindow();

    return true;
}

DWidget *MgmtWindow::initAgreementWidget()
{
    DWidget *widget = new DWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setMargin(0);
    vLayout->setSpacing(10);

    ThemedLable *label = new ThemedLable(tr("User Agreement"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Medium);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    OperatingLineWidget *oper = new OperatingLineWidget(this);
    oper->setName(tr("UOS AI User Agreement"));
    oper->setEditText(tr("Read and agreed"));

    connect(oper, &OperatingLineWidget::signalNotDeleteButtonClicked, this, [this]() {
        UserAgreementDialog dlg(this);
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
    vLayout->setMargin(0);
    vLayout->setSpacing(10);

    ThemedLable *label = new ThemedLable(tr("Proxy Settings"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Medium);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    OperatingLineWidget *oper = new OperatingLineWidget(this);
    oper->setName(tr("Proxy Settings"));
    oper->setEditText(tr("Go to settings"));
    oper->setEditHighlight(true);

    connect(oper, &OperatingLineWidget::signalNotDeleteButtonClicked, this, []() {
        DbusControlCenterRequest dbus;
        dbus.showPage("network", "System Proxy");
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

void MgmtWindow::showEx(bool showAddllmPage)
{
    //已经存在模态框，激活模态框继续操作
    auto addDlg = this->findChild<QDialog *>();
    if (addDlg && !addDlg->isHidden()) {
        addDlg->showNormal();
        addDlg->activateWindow();
        return;
    }

    if (!DbWrapper::localDbWrapper().getAICopilotIsOpen() || (showAddllmPage && DbWrapper::localDbWrapper().queryLlmList().isEmpty())) {
        this->hide();
        if (m_pWelcomeDlg->isHidden()) {
            m_pWelcomeDlg->resetDialog();
            m_pWelcomeDlg->show();
        } else {
            m_pWelcomeDlg->activateWindow();
        }
    } else {
        if (m_pLocalModelListWidget) m_pLocalModelListWidget->updateLocalModelList();
        this->activateWindow();
        this->show();
        if (showAddllmPage) {
            onAddModel();
        }
    }
}
