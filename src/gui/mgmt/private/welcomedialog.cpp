#include "welcomedialog.h"
#include "app/serverwrapper.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "wrapcheckbox.h"
#include "utils/esystemcontext.h"
#include "utils/util.h"
#include "database/providertable.h"
#include "database/modelstable.h"
#include "global_key_define.h"
#include "global_define.h"
#include "appdatabase.h"
#include "builtinprovider.h"
#include "modelvendor.h"
#include "app/application.h"

#include <DTitlebar>
#include <DLabel>

#include <DDialog>
#include <DPaletteHelper>

#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QUuid>
#include <QApplication>
#include <QLoggingCategory>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr char info[] = "uos-ai-assistant_info";
static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

WelcomeDialog::WelcomeDialog(DWidget *parent, bool onlyUseAgreement) : DAbstractDialog(parent)
  , m_freeAccount(false)
  , m_onlyUseAgreement(onlyUseAgreement)
{
    setDisplayPosition(DisplayPosition::Center);

    initUI();
    initConnect();
    resetDialog();
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
    int dialogWidth = 530;
    setFixedWidth(dialogWidth);
    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    DLabel *logoLabel = new DLabel(this);
    qreal dpr = this->devicePixelRatioF();
    QPixmap pixmap = QIcon::fromTheme(getApplicationIconName()).pixmap(QSize(48, 48) * dpr);
    pixmap.setDevicePixelRatio(dpr);
    logoLabel->setPixmap(pixmap);


    ThemedLable *titleLable = new ThemedLable(tr("Welcome to UOS AI"));
    titleLable->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.9);
    titleLable->setAlignment(Qt::AlignLeft);
    titleLable->setFixedWidth(dialogWidth * 0.87);
    {
        QFont font;
        font.setPixelSize(24);
        font.setWeight(QFont::Medium);
        titleLable->setFont(font);
    }

    m_pIntroduce = new ThemedLable(tr("UOS AI, your smart assistant, is designed to improve your productivity and enjoy a high-quality work experience."));
    m_pIntroduce->setPaletteColor(QPalette::Text, DPalette::BrightText, 0.7);
    m_pIntroduce->setFixedWidth(dialogWidth * 0.87);
    m_pIntroduce->setAlignment(Qt::AlignLeft);
    m_pIntroduce->setWordWrap(true);
    {
        QFont font;
        font.setPixelSize(16);
        font.setWeight(QFont::Normal);
        m_pIntroduce->setFont(font);
    }

    auto labelLayout = new QHBoxLayout();
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->addWidget(m_pIntroduce, 1, Qt::AlignLeft);

    m_pAgrCheckbox = new WrapCheckBox();
    m_pAgrCheckbox->setTextFormat(Qt::RichText);
    m_pAgrCheckbox->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pAgrCheckbox->setOpenExternalLinks(true);
    m_pAgrCheckbox->setTextMaxWidth(dialogWidth - 102);
    m_pAgrCheckbox->setFixedWidth(dialogWidth - 72);
    {
        QFont font;
        font.setPixelSize(12);
        font.setWeight(QFont::Normal);
        m_pAgrCheckbox->setTextFont(font);
    }

    auto agrLayout = new QHBoxLayout();
    agrLayout->setContentsMargins(0, 0, 0, 0);
    agrLayout->addWidget(m_pAgrCheckbox, 1, Qt::AlignTop | Qt::AlignLeft);
    
    m_pFreeAccount = new ThemedButton();
    m_pFreeAccount->setButtonStyle(ThemedButton::Default);
    m_pFreeAccount->setFixedSize(240, 30);
    m_pFreeAccount->setText(tr("Get a free account"));
    m_pActivity = new ThemedLable();
    m_pActivity->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.5);
    m_pActivity->setTextFormat(Qt::RichText);
    m_pActivity->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pActivity->setOpenExternalLinks(true);
    m_pActivity->setFixedWidth(240);
    m_pActivity->setAlignment(Qt::AlignCenter);
    m_pActivity->setWordWrap(true);
    {
        QFont font;
        font.setPixelSize(10);
        font.setWeight(QFont::Medium);
        m_pActivity->setFont(font);
    }

    m_pFreeWidget = new DWidget();
    auto freeLayout = new QVBoxLayout(m_pFreeWidget);
    freeLayout->setSpacing(5);
    freeLayout->setContentsMargins(0, 0, 0, 0);
    freeLayout->addWidget(m_pActivity, 0, Qt::AlignCenter);
    freeLayout->addWidget(m_pFreeAccount, 0, Qt::AlignCenter);
    m_pFreeWidget->hide();
    
    m_pAddModel = new ThemedButton();
    m_pAddModel->setButtonStyle(ThemedButton::Plain);
    m_pAddModel->setFixedHeight(30);
    m_pAddModel->setMinimumWidth(80);
    m_pAddModel->setText(tr("Add Model"));
    m_pFreeAccount->setDisabled(true);
    m_pAddModel->setDisabled(true);
    {
        QFont font;
        font.setPixelSize(14);
        font.setWeight(QFont::Normal);
        m_pAddModel->setFont(font);
    }

    m_pStartUsing = new ThemedButton();
    m_pStartUsing->setButtonStyle(ThemedButton::Default);
    m_pStartUsing->setText(tr("Start using"));
    m_pStartUsing->setDisabled(true);
    m_pStartUsing->setFixedWidth(360);

    resetLinkColor();

    // 将 m_pAddModel 放入垂直布局中，上面添加 spacer 来向下偏移，使其与 m_pFreeAccount 水平对齐
    auto addModelLayout = new QVBoxLayout();
    addModelLayout->setSpacing(0);
    addModelLayout->setContentsMargins(0, 0, 0, 0);
    addModelLayout->addSpacing(m_pActivity->sizeHint().height() + 5);
    addModelLayout->addWidget(m_pAddModel);

    m_pButtonLayout = new QHBoxLayout();
    m_pButtonLayout->setContentsMargins(0, 0, 36, 0);
    m_pButtonLayout->setSpacing(0);
    m_pButtonLayout->addLayout(addModelLayout);
    m_pButtonLayout->addStretch();
    m_pButtonLayout->addWidget(m_pFreeWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(36, 0, 0, 26);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignLeft);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(titleLable, 0, Qt::AlignLeft);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(labelLayout);
    mainLayout->addSpacing(24);
    mainLayout->addLayout(agrLayout);
    mainLayout->addSpacing(32);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pStartUsing, 0, Qt::AlignHCenter);
    mainLayout->addLayout(m_pButtonLayout);
    this->setLayout(mainLayout);
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
        QMetaObject::invokeMethod(aiApp, "showConfig", Qt::QueuedConnection, Q_ARG(int, MgmtWindow::Page::ModelList));
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

void WelcomeDialog::updateButtonLayout()
{
    if (m_pButtonLayout) {
        // Clear existing layout contents
        while (m_pButtonLayout->count() > 0) {
            QLayoutItem *item = m_pButtonLayout->takeAt(0);
            if (item) {
                delete item;
            }
        }

        // Rebuild layout based on m_pFreeWidget visibility
        if (m_pFreeWidget->isVisible()) {
            // Left-right layout: addModelLayout on left, m_pFreeWidget on right
            auto addModelLayout = new QVBoxLayout();
            addModelLayout->setSpacing(0);
            addModelLayout->setContentsMargins(0, 0, 0, 0);
            addModelLayout->addSpacing(m_pActivity->sizeHint().height() + 5);
            addModelLayout->addWidget(m_pAddModel);

            m_pButtonLayout->addLayout(addModelLayout);
            m_pButtonLayout->addStretch();
            m_pButtonLayout->addWidget(m_pFreeWidget);
        } else {
            // Right-aligned layout: addModelLayout on right
            m_pButtonLayout->addStretch();
            auto addModelLayout = new QVBoxLayout();
            addModelLayout->setSpacing(0);
            addModelLayout->setContentsMargins(0, 0, 0, 0);
            addModelLayout->addWidget(m_pAddModel);

            m_pButtonLayout->addLayout(addModelLayout);
        }
    }
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
    m_pActivity->setText(tr("Receiving an account indicates that you understand and agree to the terms of the event,<a href=\"%1\" style=\"color:%2; text-decoration:none;\">Event Details</a>").arg(m_activityUrl).arg(tmpC.name()));
}

void WelcomeDialog::updateAgree()
{
    if (m_pAgrCheckbox->isEnabled()) {
        bool agreed = m_pAgrCheckbox->checkState() != Qt::Unchecked;
        AppDatabase::instance()->saveConfigBool(CONFIG_APP_AGREEMENT, agreed);
        qCInfo(logAIGUI) << "User agreement status updated:" << agreed;

        // 使用欢迎界面后无需再提示新手引导。TODO
        // {
        //     auto cur = DbWrapper::localDbWrapper().getGuideKey();
        //     if (cur.isEmpty() || cur.toInt() < DbWrapper::builtinGuideVersion()) {
        //         DbWrapper::localDbWrapper().updateGuideKey(QString::number(DbWrapper::builtinGuideVersion()));
        //         qCDebug(logAIGUI) << "Guide version updated to:" << DbWrapper::builtinGuideVersion();
        //     }
        // }
    }
}

bool WelcomeDialog::isFreeAccount()
{
    return m_freeAccount;
}

void WelcomeDialog::onGetFreeAccount()
{
    for ( const ProviderAccount &provider : AppDatabase::instance()->queryAllProviders().values()) {
        if (ModelVendor::isUosProvider(provider.provider)) {
            qCWarning(logAIGUI) << "UOS AI free account already exists.";

            updateAgree();
            accept();
            return;
        }
    }

    qCDebug(logAIGUI) << "Requesting free account";
    m_pFreeAccount->setDisabled(true);

    DDialog dlg(this);
    dlg.setIcon(QIcon(WARNING_ICON));
    dlg.setMinimumWidth(380);

    UosFreeAccount freeAccount;
    int status;
    QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(1, 83, freeAccount, status);

    m_pFreeAccount->setDisabled(false);
    if (QNetworkReply::NoError == error) {
        qCInfo(logAIGUI) << "Free account received successfully";
        m_freeAccount = true;

        QJsonObject authObj;
        authObj[STR_KEY_API_KEY] = freeAccount.appkey;

        auto provider = ProviderTable::create(
                    GlobalUtil::generateUuid(),
                    tr("UOS AI Trial Account"),
                    QString::fromUtf8(QJsonDocument(authObj).toJson(QJsonDocument::Compact)),
                    STR_KEY_UOS_AI,
                    QString()
                    );

        if (!provider.save(AppDatabase::instance())) {
            qCWarning(logAIGUI) << "Failed to save free account provdier to local DB.";
            dlg.setMessage(tr("Save failed, please try again later"));
            dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }

        ModelsTable table = ModelsTable::create(
                    GlobalUtil::generateUuid(),
                    provider.id(),
                    ModelsTable::createModel(UOS_FREE_MODEL_AUTO)
                    );

        if (!table.save(AppDatabase::instance())) {
            qCWarning(logAIGUI) << "Failed to save free account provdier to local DB.";
            dlg.setMessage(tr("Save failed, please try again later"));
            dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }

        qCInfo(logAIGUI) << "Free account saved to local DB.";

        // 刷新账号库
        ModelVendor::instance()->refresh();

        qCInfo(logAIGUI) << "Free account configuration saved successfully";
        emit freeModelAppend();

        dlg.setTitle(tr("Trial account received successfully."));// 小U同学试用账号领取成功！

        if (Util::checkLanguage())
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
        updateButtonLayout();
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
    {
        QFont font;
        font.setPixelSize(12);
        font.setWeight(QFont::Medium);
        pExpContent->setFont(font);
    }
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
        updateButtonLayout();
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
                QSharedPointer<UosFreeAccountActivity> tmpActivity(new UosFreeAccountActivity);
                QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
                    return UosFreeAccounts::instance().freeAccountButtonDisplay("account", *tmpActivity.data());
                });
                m_watcher->setFuture(future);
                connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]()
                {
                    m_hasActivity = *tmpActivity.data();
                    if (QNetworkReply::NoError == m_watcher.data()->future().result() && 0 != m_hasActivity.display && !m_onlyUseAgreement) {
                        m_pFreeWidget->show();
                        m_pFreeAccount->show();
                        m_pActivity->show();
                        m_activityUrl = m_hasActivity.url;
                        resetLinkColor();
                        updateButtonLayout();
                    } else {
                        m_pFreeWidget->hide();
                        updateButtonLayout();
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
    return AppDatabase::instance()->getConfigBool(CONFIG_APP_AGREEMENT);
}

