#include "welcomedialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "wrapcheckbox.h"
#include "private/echatwndmanager.h"

#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QUuid>
#include <QApplication>
#include <QDesktopWidget>

static constexpr char info[] = "uos-ai-assistant_info";
static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

WelcomeDialog::WelcomeDialog(DWidget *parent, bool onlyUseAgreement) : DAbstractDialog(parent)
  , m_freeAccount(false)
  , m_onlyUseAgreement(onlyUseAgreement)
{
    EWndManager()->registeWindow(this);

    initUI();
    initConnect();
    setModal(true);
    resetDialog();
}

void WelcomeDialog::initUI()
{
    setFixedWidth(520);
    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    ThemedLable *titleLable = new ThemedLable(tr("Welcome to UOS AI"));
    titleLable->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.9);
    titleLable->setAlignment(Qt::AlignCenter);
    titleLable->setFixedWidth(468);
    DFontSizeManager::instance()->bind(titleLable, DFontSizeManager::T6, QFont::Medium);

    ThemedLable *label = new ThemedLable(tr("UOS AI, your smart assistant, is designed to improve your productivity and enjoy a high-quality work experience."));
    label->setPaletteColor(QPalette::Text, DPalette::BrightText, 0.7);
    label->setWordWrap(true);
    label->setFixedWidth(468);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T6, QFont::Normal);

    m_pAgrCheckbox = new WrapCheckBox();
    m_pAgrCheckbox->setTextFormat(Qt::RichText);
    m_pAgrCheckbox->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pAgrCheckbox->setOpenExternalLinks(true);
    m_pAgrCheckbox->setTextMaxWidth(438);

    auto agrLayout = new QHBoxLayout();
    agrLayout->setContentsMargins(26, 0, 26, 0);
    agrLayout->setSpacing(10);
    agrLayout->addWidget(m_pAgrCheckbox, 0, Qt::AlignTop | Qt::AlignLeft);
    agrLayout->addStretch();

    m_pExpCheckbox = new WrapCheckBox();
    m_pExpCheckbox->setTextMaxWidth(410);
    m_pExpCheckbox->setText(tr("I agree to participate in the user experience plan of the Application"));

    m_pExpIcon = new DIconButton(static_cast<QStyle::StandardPixmap>(-1), this);
    m_pExpIcon->setFixedSize(QSize(20, 20));
    m_pExpIcon->setIcon(QIcon::fromTheme(info));
    m_pExpIcon->setIconSize(QSize(20, 20));
    m_pExpIcon->installEventFilter(this);

    auto iconLayout = new QVBoxLayout;
    iconLayout->setContentsMargins(0, 2, 0, 0);
    iconLayout->addWidget(m_pExpIcon);
    iconLayout->addStretch();

    auto expLayout = new QHBoxLayout();
    expLayout->setContentsMargins(26, 0, 26, 0);
    expLayout->setSpacing(5);
    expLayout->addWidget(m_pExpCheckbox);
    expLayout->addLayout(iconLayout);
    expLayout->addStretch();
    expLayout->addStretch();

    m_pFreeAccount = new DSuggestButton();
    m_pFreeAccount->setFixedSize(310, 36);
    m_pFreeAccount->setText(tr("Get a free trial account"));
    m_pActivity = new ThemedLable();
    m_pActivity->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.5);
    m_pActivity->setTextFormat(Qt::RichText);
    m_pActivity->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pActivity->setOpenExternalLinks(true);
    DFontSizeManager::instance()->bind(m_pActivity, DFontSizeManager::T10, QFont::Medium);
    m_pFreeWidget = new DWidget();
    auto freeLayout = new QVBoxLayout(m_pFreeWidget);
    freeLayout->setSpacing(5);
    freeLayout->setContentsMargins(0, 0, 0, 10);
    freeLayout->addWidget(m_pFreeAccount, 0, Qt::AlignCenter);
    freeLayout->addWidget(m_pActivity, 0, Qt::AlignCenter);
    m_pFreeWidget->hide();

    m_pAddModel = new DPushButton();
    m_pAddModel->setFixedSize(310, 36);
    m_pAddModel->setText(tr("Configure the model account"));
    m_pFreeAccount->setDisabled(true);
    m_pAddModel->setDisabled(true);

    m_pStartUsing = new DSuggestButton();
    m_pStartUsing->setText(tr("Start using"));
    m_pStartUsing->setDisabled(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 20);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(titleLable, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(label, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(agrLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(expLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_pStartUsing, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_pFreeWidget);
    mainLayout->addWidget(m_pAddModel, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
    this->setLayout(mainLayout);

    resetLinkColor();
}

void WelcomeDialog::initConnect()
{
    connect(m_pAgrCheckbox, &WrapCheckBox::stateChanged, this, [this ](int state) {
        m_pFreeAccount->setDisabled(state == Qt::Unchecked);
        m_pAddModel->setDisabled(state == Qt::Unchecked);
        m_pStartUsing->setDisabled(state == Qt::Unchecked);
    });
    connect(m_pAddModel, &DPushButton::clicked, this, &WelcomeDialog::accept);
    connect(m_pStartUsing, &DPushButton::clicked, this, &WelcomeDialog::accept);
    connect(m_pFreeAccount, &DPushButton::clicked, this, &WelcomeDialog::onGetFreeAccount);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WelcomeDialog::onUpdateSystemTheme);
}

void WelcomeDialog::onUpdateSystemFont(const QFont &)
{
    this->adjustSize();
}

void WelcomeDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    resetLinkColor();
}

void WelcomeDialog::resetLinkColor()
{
    QColor tmpC = DApplicationHelper::instance()->palette(m_pAgrCheckbox).color(DPalette::Normal, DPalette::Highlight);
    if (QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script()) {
        m_pAgrCheckbox->setText(tr("I confirm that I am over 18 years old. I acknowledge and agree to the <a href=\"%1\" style=\"color: %2; text-decoration: none;\">\"UOS AI User Agreement\"</a>, and the contents I send and receive via the Application are direct data exchanges with the large model service provider and have nothing to do with the Company.").arg("https://uosai.uniontech.com/agreement/UOSAIUserAgreement_CH.html").arg(tmpC.name()));
    } else {
        m_pAgrCheckbox->setText(tr("I confirm that I am over 18 years old. I acknowledge and agree to the <a href=\"%1\" style=\"color: %2; text-decoration: none;\">\"UOS AI User Agreement\"</a>, and the contents I send and receive via the Application are direct data exchanges with the large model service provider and have nothing to do with the Company.").arg("https://uosai.uniontech.com/agreement/UOSAIUserAgreement_EN.html").arg(tmpC.name()));
    }
    m_pActivity->setText(tr("Receiving an account indicates that you understand and agree to the terms of the event,<a href=\"%1\" style=\"color:%2; text-decoration:none;\">Event Details></a>").arg(m_activityUrl).arg(tmpC.name()));
}

bool WelcomeDialog::isFreeAccount()
{
    return m_freeAccount;
}

Qt::CheckState WelcomeDialog::getUserExpState()
{
    return m_pExpCheckbox->checkState();
}

void WelcomeDialog::onGetFreeAccount()
{
    m_pFreeAccount->setDisabled(true);

    DDialog dlg(this);
    dlg.setIcon(QIcon(WARNING_ICON));

    UosFreeAccount freeAccount;
    int status;
    QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, freeAccount, status);

    m_pFreeAccount->setDisabled(false);
    if (QNetworkReply::NoError == error) {
        m_freeAccount = true;

        LLMServerProxy llm;
        llm.type = ModelType::FREE_NORMAL;
        llm.id = freeAccount.appkey;
        llm.model = static_cast<LLMChatModel>(freeAccount.llmModel);
        llm.url = freeAccount.modelUrl;
        llm.name = LLMServerProxy::llmName(llm.model, !llm.url.isEmpty()) + "-" + tr("Trial Account");
        AccountProxy accountProxy;
        SocketProxy socketProxy;
        socketProxy.socketProxyType = SocketProxyType::SYSTEM_PROXY;
        accountProxy.socketProxy = socketProxy;
        accountProxy.appId = freeAccount.appid;
        accountProxy.apiKey = freeAccount.appkey;
        accountProxy.apiSecret = freeAccount.appsecret;
        llm.account = accountProxy;

        if (!DbWrapper::localDbWrapper().appendLlm(llm)) {
            dlg.setMessage(tr("Save failed, please try again later"));
            dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }

        ServerWrapper::instance()->updateLLMAccount();
        emit signalAppendModel(llm);
        dlg.setTitle("UOS AI试用账号领取成功！");
        dlg.setMessage(tr("The number of uses and duration of the trial account are limited, please configure your personal model account in time! See event details for details."));
        dlg.addButton(tr("Start trial", "button"), true, DDialog::ButtonNormal);

        dlg.exec();

        accept();
    } else if (1 == status) {
        dlg.setMessage(tr("The free account activity ends."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);

        dlg.exec();

        m_pFreeWidget->hide();
        this->adjustSize();
    } else {
        dlg.setMessage(tr("Unable to connect to the server, please check your network or try again later."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
        dlg.exec();
    }
}

DArrowRectangle *WelcomeDialog::showArrowRectangle(DArrowRectangle::ArrowDirection direction)
{
    auto pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWidget, this);
    pExpTips->setRadiusArrowStyleEnable(true);
    pExpTips->setRadius(16);
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Base);
    color.setAlphaF(0.3);
    pExpTips->setBackgroundColor(color);
    pExpTips->setMargin(15);

    auto pExpContent = new DLabel;
    pExpContent->setText(tr("I agree to participate in the user experience plan of the Application, and authorize your company to collect the contents I send while using the Application, the time of sending, the type of requested large model ，the specific application and whether the text generated the image successfully, so as to improve the service quality and enhance the operation experience. (If you refuse to provide the above information, it will not affect your normal use of the Application.)"));
    pExpContent->setWordWrap(true);
    pExpContent->setFixedWidth(250);
    QPalette pl = pExpContent->palette();
    pl.setColor(QPalette::Text, DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Text));
    pExpContent->setPalette(pl);
    pExpContent->setForegroundRole(QPalette::Text);
    pExpContent->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(pExpContent, DFontSizeManager::T8, QFont::Medium);
    pExpTips->setContent(pExpContent);

    return pExpTips;
}

bool WelcomeDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pExpIcon) {
        if (event->type() == QEvent::Enter) {
            //获取当前鼠标位置
            QPoint curPos = QCursor::pos();
            //获取屏幕大小
            QRect desktopRect = QApplication::desktop()->rect();
            //根据提示框在屏幕的位置设置箭头方向
            QPoint p = mapToGlobal(m_pExpIcon->pos());
            if (curPos.x() + 300 > desktopRect.width()) {
                auto tips = showArrowRectangle(DArrowRectangle::ArrowRight);
                tips->show(p.x(), p.y() + 10);
            } else {
                auto tips = showArrowRectangle(DArrowRectangle::ArrowLeft);
                tips->show(p.x() + 20, p.y() + 10);
            }
        } else if (event->type() == QEvent::Leave) {
            auto tips = this->findChild<DArrowRectangle *>();
            if (tips) tips->deleteLater();
        } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
    }

    return DAbstractDialog::eventFilter(watched, event);
}

void WelcomeDialog::resetDialog()
{
    m_freeAccount = false;
    if (QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script()) {
        m_pExpCheckbox->setCheckState(DbWrapper::localDbWrapper().getUserExpState() >= 0 ? Qt::Checked : Qt::Unchecked);
    } else {
        m_pExpCheckbox->setCheckState(DbWrapper::localDbWrapper().getUserExpState() > 0 ? Qt::Checked : Qt::Unchecked);
    }
     m_pAgrCheckbox->setCheckState(DbWrapper::localDbWrapper().getAICopilotIsOpen() ? Qt::Checked : Qt::Unchecked);
     if (DbWrapper::localDbWrapper().getAICopilotIsOpen())
         m_pAgrCheckbox->setDisabled(true);

    //中文环境下检测免费账号活动
    if (QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script()) {
        QTimer::singleShot(100, this, [this] {
            m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
            QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
                return UosFreeAccounts::instance().freeAccountButtonDisplay("account", m_hasActivity);
            });
            m_watcher->setFuture(future);
            connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]()
            {
                if (QNetworkReply::NoError == m_watcher.data()->future().result() && 0 != m_hasActivity.display) {
                    m_pFreeWidget->show();
                    this->adjustSize();
                    m_activityUrl = m_hasActivity.url;
                    resetLinkColor();
                }
            });
        });
    }

    if (m_onlyUseAgreement) {
        m_pExpCheckbox->hide();
        m_pExpIcon->hide();
        m_pFreeWidget->hide();
        m_pFreeAccount->hide();
        m_pActivity->hide();
        m_pAddModel->hide();

        m_pStartUsing->show();
    } else {
        m_pStartUsing->hide();

        m_pExpCheckbox->show();
        m_pExpIcon->show();
        m_pFreeAccount->show();
        m_pActivity->show();
        m_pFreeWidget->show();
        m_pAddModel->show();
    }
}
