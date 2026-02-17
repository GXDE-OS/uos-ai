#include "welcomedialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "wrapcheckbox.h"
#include "private/echatwndmanager.h"
#include "utils/esystemcontext.h"
#include "private/eaiexecutor.h"
#include "utils/util.h"

#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DDialog>
#include <DPaletteHelper>

#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QUuid>
#include <QApplication>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr char info[] = "uos-ai-assistant_info";
static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

WelcomeDialog::WelcomeDialog(DWidget *parent, bool onlyUseAgreement) : DAbstractDialog(parent)
  , m_freeAccount(false)
  , m_onlyUseAgreement(onlyUseAgreement)
{
    EWndManager()->registeWindow(this);
    setDisplayPosition(DisplayPosition::Center);

    initUI();
    initConnect();
    setModal(true);
}

WelcomeDialog *WelcomeDialog::instance(bool onlyUseAgreement)
{
    static WelcomeDialog instance(nullptr, onlyUseAgreement);
    instance.setOnlyUseAgreement(onlyUseAgreement);
    if (!instance.isVisible())
        instance.resetDialog();
    else
        instance.activateWindow();

    return &instance;
}

void WelcomeDialog::initUI()
{
    int dialogWidth = 460;
    setFixedWidth(dialogWidth);
    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    DLabel *logoLabel = new DLabel(this);
    logoLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant").pixmap(90, 90));

    ThemedLable *titleLable = new ThemedLable(tr("Welcome to UOS AI"));
    titleLable->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.9);
    titleLable->setAlignment(Qt::AlignCenter);
    titleLable->setFixedWidth(dialogWidth * 0.75);
    DFontSizeManager::instance()->bind(titleLable, DFontSizeManager::T3, QFont::Medium);

    m_pIntroduce = new ThemedLable(tr("UOS AI, your smart assistant, is designed to improve your productivity and enjoy a high-quality work experience."));
    m_pIntroduce->setPaletteColor(QPalette::Text, DPalette::BrightText, 0.7);
    m_pIntroduce->setFixedWidth(dialogWidth * 0.75);
    m_pIntroduce->setAlignment(Qt::AlignCenter);
    m_pIntroduce->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pIntroduce, DFontSizeManager::T5, QFont::Normal);

    auto labelLayout = new QHBoxLayout();
    labelLayout->setContentsMargins(50, 0, 50, 0);
    labelLayout->addWidget(m_pIntroduce, 1, Qt::AlignCenter);

    m_pAgrCheckbox = new WrapCheckBox();
    m_pAgrCheckbox->setTextFormat(Qt::RichText);
    m_pAgrCheckbox->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pAgrCheckbox->setOpenExternalLinks(true);
    m_pAgrCheckbox->setTextMaxWidth(dialogWidth - 80);
    m_pAgrCheckbox->setFixedWidth(dialogWidth - 40);
    m_pAgrCheckbox->setFontSize(DFontSizeManager::T7, QFont::Normal);

    auto agrLayout = new QHBoxLayout();
    agrLayout->setContentsMargins(20, 0, 20, 0);
    agrLayout->addWidget(m_pAgrCheckbox, 1, Qt::AlignTop | Qt::AlignLeft);

    m_pFreeAccount = new DSuggestButton();
    m_pFreeAccount->setFixedSize(360, 36);
    m_pFreeAccount->setText(tr("Get a free trial account"));
    m_pActivity = new ThemedLable();
    m_pActivity->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.5);
    m_pActivity->setTextFormat(Qt::RichText);
    m_pActivity->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pActivity->setOpenExternalLinks(true);
    m_pActivity->setFixedWidth(dialogWidth - 40);
    m_pActivity->setAlignment(Qt::AlignCenter);
    m_pActivity->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pActivity, DFontSizeManager::T10, QFont::Medium);

    auto activityLabelLayout = new QHBoxLayout();
    activityLabelLayout->setContentsMargins(20, 0, 20, 0);
    activityLabelLayout->addWidget(m_pActivity, 1, Qt::AlignCenter);

    m_pFreeWidget = new DWidget();
    auto freeLayout = new QVBoxLayout(m_pFreeWidget);
    freeLayout->setSpacing(5);
    freeLayout->setContentsMargins(0, 0, 0, 10);
    freeLayout->addWidget(m_pFreeAccount, 0, Qt::AlignCenter);
    freeLayout->addLayout(activityLabelLayout);
    m_pFreeWidget->hide();


    m_pAddModel = new DPushButton();
    m_pAddModel->setFixedSize(360, 36);
    m_pAddModel->setText(tr("Add Model"));
    m_pFreeAccount->setDisabled(true);
    m_pAddModel->setDisabled(true);

    m_pStartUsing = new DSuggestButton();
    m_pStartUsing->setText(tr("Start using"));
    m_pStartUsing->setDisabled(true);
    m_pStartUsing->setFixedWidth(360);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 20);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(titleLable, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(labelLayout);
    m_pVerticalSpacer = new QSpacerItem(0, 100, QSizePolicy::Minimum, QSizePolicy::Fixed);
    mainLayout->addSpacerItem(m_pVerticalSpacer);
    mainLayout->addStretch();
    mainLayout->addLayout(agrLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_pStartUsing, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_pFreeWidget);
    mainLayout->addWidget(m_pAddModel, 0, Qt::AlignHCenter);
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

    connect(m_pAddModel, &DPushButton::clicked, this, [this ](int state) {
        updateAgree();
        emit signalShowMgmtWindowAfterChatInitFinished();
    });

    connect(m_pAddModel, &DPushButton::clicked, this, &WelcomeDialog::accept);

    connect(m_pStartUsing, &DPushButton::clicked, this, &WelcomeDialog::accept);
    connect(m_pFreeAccount, &DPushButton::clicked, this, &WelcomeDialog::onGetFreeAccount);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WelcomeDialog::onUpdateSystemTheme);
}

void WelcomeDialog::onUpdateSystemFont(const QFont &)
{
    this->setMinimumHeight(0);
    this->setMaximumWidth(QWIDGETSIZE_MAX);
    this->adjustSize();

    int heightDiff = 0;
    heightDiff += m_pIntroduce->sizeHint().height() - m_pIntroduce->height();
    heightDiff += m_pAgrCheckbox->sizeHint().height() - m_pAgrCheckbox->height();
    heightDiff += m_pActivity->sizeHint().height() - m_pActivity->height();
    if(heightDiff > 0) {
        int height = this->height() + heightDiff + 50;
        if (auto screen = QGuiApplication::primaryScreen()) {
            const int screenHeight = screen->availableGeometry().height();
            if (height > screenHeight) {
                const int overflow = height - screenHeight;
                if (m_pVerticalSpacer) {
                    m_pVerticalSpacer->changeSize(0, 100 - overflow, QSizePolicy::Minimum, QSizePolicy::Fixed);
                    layout()->invalidate();
                }
                height = screenHeight;
            } else {
                if (m_pVerticalSpacer) {
                    m_pVerticalSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
                }
            }
        }
        this->setFixedHeight(height);
    } else {
        this->setMinimumHeight(0);
        this->setMaximumWidth(QWIDGETSIZE_MAX);
    }
    this->adjustSize();
}

void WelcomeDialog::showEvent(QShowEvent *event)
{
    return DAbstractDialog::showEvent(event);
}

void WelcomeDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    resetLinkColor();
}

void WelcomeDialog::resetLinkColor()
{
    QColor tmpC = DPaletteHelper::instance()->palette(m_pAgrCheckbox).color(DPalette::Normal, DPalette::Highlight);
    if (QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script()) {
        m_pAgrCheckbox->setText(tr("I confirm that I am over 18 years old. I acknowledge and agree to the <a href=\"%1\" style=\"color: %2; text-decoration: none;\">\"UOS AI User Agreement\"</a>, and the contents I send and receive via the Application are direct data exchanges with the large model service provider and have nothing to do with the Company.").arg("https://uosai.uniontech.com/agreement/UOSAIUserAgreement_CH.html").arg(tmpC.name()));
    } else {
        m_pAgrCheckbox->setText(tr("I confirm that I am over 18 years old. I acknowledge and agree to the <a href=\"%1\" style=\"color: %2; text-decoration: none;\">\"UOS AI User Agreement\"</a>, and the contents I send and receive via the Application are direct data exchanges with the large model service provider and have nothing to do with the Company.").arg("https://uosai.uniontech.com/agreement/UOSAIUserAgreement_EN.html").arg(tmpC.name()));
    }
    m_pActivity->setText(tr("Receiving an account indicates that you understand and agree to the terms of the event,<a href=\"%1\" style=\"color:%2; text-decoration:none;\">Event Details></a>").arg(m_activityUrl).arg(tmpC.name()));
}

void WelcomeDialog::updateAgree()
{
    if (m_pAgrCheckbox->isEnabled()) {
        bool agreed = m_pAgrCheckbox->checkState() != Qt::Unchecked;
        DbWrapper::localDbWrapper().updateAICopilot(agreed);
        qCInfo(logAIGUI) << "User agreement status updated:" << agreed;

        // 使用欢迎界面后无需再提示新手引导。
        {
            auto cur = DbWrapper::localDbWrapper().getGuideKey();
            if (cur.isEmpty() || cur.toInt() < DbWrapper::builtinGuideVersion()) {
                DbWrapper::localDbWrapper().updateGuideKey(QString::number(DbWrapper::builtinGuideVersion()));
                qCDebug(logAIGUI) << "Guide version updated to:" << DbWrapper::builtinGuideVersion();
            }
        }
    }
}

bool WelcomeDialog::isFreeAccount()
{
    return m_freeAccount;
}

void WelcomeDialog::onGetFreeAccount()
{
    qCDebug(logAIGUI) << "Requesting free account";
    m_pFreeAccount->setDisabled(true);

    DDialog dlg(this);
    dlg.setIcon(QIcon(WARNING_ICON));
    dlg.setMinimumWidth(380);

    UosFreeAccount freeAccount;
    int status;
    QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, DeepSeek_Uos_Free, freeAccount, status);

    m_pFreeAccount->setDisabled(false);
    if (QNetworkReply::NoError == error) {
        qCInfo(logAIGUI) << "Free account received successfully";
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
            qCWarning(logAIGUI) << "Failed to save free account configuration";
            dlg.setMessage(tr("Save failed, please try again later"));
            dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }

        qCInfo(logAIGUI) << "Free account configuration saved successfully";
        ServerWrapper::instance()->updateLLMAccount();
        emit signalAppendModel(llm);
        dlg.setTitle(tr("Trial account received successfully."));// UOS AI试用账号领取成功！

        if (UOSAI_NAMESPACE::Util::checkLanguage())
            dlg.setMessage(tr("The number of uses and duration of the trial account are limited, please configure your personal model account in time! See event details for details."));
        else
            dlg.setMessage(tr("The English support for trial accounts is not satisfactory. And the number of uses and duration of the trial account are limited. Please configure your personal model account in time!"));
        dlg.addButton(tr("Start trial", "button"), true, DDialog::ButtonNormal);
        dlg.exec();

        updateAgree();
        accept();
    } else if (1 == status) {
        qCInfo(logAIGUI) << "Free account activity has ended";
        dlg.setMessage(tr("The free account activity ends."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
        dlg.exec();

        m_pFreeWidget->hide();
        this->adjustSize();
    } else {
        qCWarning(logAIGUI) << "Failed to connect to server for free account, error:" << error << "status:" << status;
        dlg.setMessage(tr("Unable to connect to the server, please check your network or try again later."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
        dlg.exec();
    }
}

DArrowRectangle *WelcomeDialog::showArrowRectangle(DArrowRectangle::ArrowDirection direction)
{
    DArrowRectangle *pExpTips = nullptr;
    if (ESystemContext::isWayland())
        pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWidget, this);
    else
        pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWindow, this);
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

void WelcomeDialog::resetDialog()
{
    m_freeAccount = false;

    {
        bool agreed = isAgreed();
        qCDebug(logAIGUI) << "Current agreement status:" << agreed;
        m_pAgrCheckbox->setCheckState(agreed ? Qt::Checked : Qt::Unchecked);
        if (agreed)
            m_pAgrCheckbox->setDisabled(true);
    }

    if (m_onlyUseAgreement) {
        m_pFreeWidget->hide();
        m_pFreeAccount->hide();
        m_pActivity->hide();
        m_pAddModel->hide();

        m_pStartUsing->show();

        this->adjustSize();
    } else {
        m_pStartUsing->hide();
        m_pAddModel->show();

        //中文环境下检测免费账号活动
#ifdef ENABLE_FREEACCOUNT
        if (true) {
#else
        if (UOSAI_NAMESPACE::Util::checkLanguage()) {
#endif
            QTimer::singleShot(100, this, [this] {
                if (m_watcher && m_watcher->isRunning())
                    return;
                m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
                QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
                    return UosFreeAccounts::instance().freeAccountButtonDisplay("account", m_hasActivity);
                });
                m_watcher->setFuture(future);
                connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]()
                {
                    if (QNetworkReply::NoError == m_watcher.data()->future().result() && 0 != m_hasActivity.display && !m_onlyUseAgreement) {
                        m_pFreeWidget->show();
                        m_pFreeAccount->show();
                        m_pActivity->show();
                        m_activityUrl = m_hasActivity.url;
                        resetLinkColor();
                    } else {
                        m_pFreeWidget->hide();
                    }
                    this->adjustSize();
                    this->onUpdateSystemFont(QFont());
                });
            });
        } else {
            this->adjustSize();
            this->onUpdateSystemFont(QFont());
        }
    }
}
bool WelcomeDialog::isAgreed()
{
    return DbWrapper::localDbWrapper().getAICopilotIsOpen();
}

