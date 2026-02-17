#include "getfreeaccountdialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "themedlable.h"
#include "uosfreeaccounts.h"
#include "wrapcheckbox.h"
#include "private/echatwndmanager.h"
#include "utils/esystemcontext.h"
#include "util.h"

#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DDialog>
#include <DPaletteHelper>

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

GetFreeAccountDialog::GetFreeAccountDialog(DWidget *parent) : DDialog(parent)
  , m_freeAccount(false)
{
    EWndManager()->registeWindow(this);

    initUI();
    initConnect();
    setModal(true);
    resetDialog();
}

void GetFreeAccountDialog::initUI()
{
    this->setTitle(tr("Get a free trial account"));
    this->setFixedWidth(380);
    m_pActivity = new ThemedLable();
    m_pActivity->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.5);
    m_pActivity->setTextFormat(Qt::RichText);
    m_pActivity->setAlignment(Qt::AlignCenter);
    m_pActivity->setWordWrap(true);
    m_pActivity->setFixedWidth(340);
    m_pActivity->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_pActivity->setOpenExternalLinks(true);
    DFontSizeManager::instance()->bind(m_pActivity, DFontSizeManager::T6, QFont::Medium);

    this->addContent(m_pActivity);
    this->setSpacing(5);

    resetLinkColor();

    this->addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    this->addButton(tr("Get account", "button"), true, DDialog::ButtonRecommend);
    adjustSize();
}

void GetFreeAccountDialog::initConnect()
{
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &GetFreeAccountDialog::onUpdateSystemTheme);
}


void GetFreeAccountDialog::onUpdateSystemFont(const QFont &)
{
    qCDebug(logAIGUI) << "System font changed, adjusting dialog size.";
    this->adjustSize();
}

void GetFreeAccountDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    qCDebug(logAIGUI) << "System theme changed, resetting link color.";
    resetLinkColor();
}

void GetFreeAccountDialog::resetLinkColor()
{
    qCDebug(logAIGUI) << "Resetting link color for activity label.";
    QColor tmpC = DPaletteHelper::instance()->palette(m_pActivity).color(DPalette::Normal, DPalette::Highlight);
    m_pActivity->setText(tr("Receiving an account indicates that you understand and agree to the terms of the event,"
                            "<a href=\"%1\" style=\"color:%2; text-decoration:none;\">Event Details></a>").arg(m_activityUrl).arg(tmpC.name()));//"https://www.deepin.org/zh/ai-free-experience/"m_activityUrl

}

bool GetFreeAccountDialog::isFreeAccount()
{
    return m_freeAccount;
}

void GetFreeAccountDialog::onGetFreeAccount()
{
    qCInfo(logAIGUI) << "User attempts to get a free account.";
//    m_pFreeAccount->setDisabled(true);

    DDialog dlg(this);
    dlg.setIcon(QIcon(WARNING_ICON));

    UosFreeAccount freeAccount;
    int status;
    QNetworkReply::NetworkError error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, DeepSeek_Uos_Free, freeAccount, status);

//    m_pFreeAccount->setDisabled(false);
    if (QNetworkReply::NoError == error) {
        m_freeAccount = true;
        qCInfo(logAIGUI) << "Free account received successfully. AppKey:" << freeAccount.appkey;

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
            qCWarning(logAIGUI) << "Failed to save free account to local DB.";
            dlg.setMessage(tr("Save failed, please try again later"));
            dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
            dlg.exec();
            return;
        }
        qCInfo(logAIGUI) << "Free account saved to local DB.";

        ServerWrapper::instance()->updateLLMAccount();
        emit signalAppendModel(llm);
        dlg.setTitle(tr("Trial account received successfully."));
        if (UOSAI_NAMESPACE::Util::checkLanguage())
            dlg.setMessage(tr("The number of uses and duration of the trial account are limited, please configure your personal model account in time! See event details for details."));
        else
            dlg.setMessage(tr("The English support for trial accounts is not satisfactory. And the number of uses and duration of the trial account are limited. Please configure your personal model account in time!"));
        dlg.addButton(tr("Start trial", "button"), true, DDialog::ButtonNormal);
        dlg.exec();
        accept();
    } else if (1 == status) {
        qCWarning(logAIGUI) << "Free account activity has ended.";
        dlg.setMessage(tr("The free account activity ends."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);

        dlg.exec();
        emit signalActivityEnd();

        this->adjustSize();
    } else {
        qCCritical(logAIGUI) << "Failed to get free account. Network error or server unavailable.";
        dlg.setMessage(tr("Unable to connect to the server, please check your network or try again later."));
        dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonNormal);
        dlg.exec();
    }
}

void GetFreeAccountDialog::resetDialog()
{
    qCInfo(logAIGUI) << "Resetting GetFreeAccountDialog.";
    m_freeAccount = false;
#ifdef ENABLE_FREEACCOUNT
    {
#else
    if (UOSAI_NAMESPACE::Util::checkLanguage()) {
#endif
        QTimer::singleShot(100, this, [this] {
            m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
            QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
                return UosFreeAccounts::instance().freeAccountButtonDisplay("account", m_hasActivity);
            });
            m_watcher->setFuture(future);
            connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]()
            {
                if (QNetworkReply::NoError == m_watcher.data()->future().result() && 0 != m_hasActivity.display) {
                    qCInfo(logAIGUI) << "Free account activity available. URL:" << m_hasActivity.url;
                    m_pActivity->show();
                    this->adjustSize();
                    m_activityUrl = m_hasActivity.url;
                    resetLinkColor();
                } else {
                    qCWarning(logAIGUI) << "No free account activity available or failed to fetch activity info.";
                    this->close();
                }
            });
        });
    }
}
