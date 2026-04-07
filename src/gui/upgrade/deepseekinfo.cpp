// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deepseekinfo.h"
#include "util.h"
#include "dbs/dbwrapper.h"
#include "uosfreeaccounts.h"
#include "gui/mgmt/private/themedlable.h"
#include "gui/gutils.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DPaletteHelper>
#include <DTitlebar>

#include <QThread>
#include <QApplication>
#include <QCloseEvent>
#include <QtConcurrent>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

DWIDGET_USE_NAMESPACE

bool DeepSeekInfo::isDialogOpen = false;

DeepSeekInfo::DeepSeekInfo(QWidget *parent) : DAbstractDialog(parent)
{
    initUI();
    initConnect();
}

DeepSeekInfo &DeepSeekInfo::getInstance()
{
    static DeepSeekInfo dlg;
    return dlg;
}

void DeepSeekInfo::initUI()
{
    setFixedWidth(400);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    DLabel *logoLabel = new DLabel(this);
    logoLabel->setScaledContents(true);
    logoLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant").pixmap(85, 85));

    m_pFirstIntroduce = new ThemedLable(this);
    m_pFirstIntroduce->setPaletteColor(QPalette::Text, DPalette::BrightText, 1);
    m_pFirstIntroduce->setFixedWidth(300);
    m_pFirstIntroduce->setAlignment(Qt::AlignCenter);
    m_pFirstIntroduce->setWordWrap(true);
    m_pFirstIntroduce->font().pixelSize();
    DFontSizeManager::instance()->bind(m_pFirstIntroduce, DFontSizeManager::T4, QFont::Medium);
    QFont font = m_pFirstIntroduce->font();
    font.setPixelSize(font.pixelSize() + 2);
    m_pFirstIntroduce->setFont(font);

    auto firstIntrlabelLayout = new QHBoxLayout();
    firstIntrlabelLayout->setContentsMargins(20, 0, 20, 0);
    firstIntrlabelLayout->addStretch();
    firstIntrlabelLayout->addWidget(m_pFirstIntroduce, 0, Qt::AlignCenter);
    firstIntrlabelLayout->addStretch();

    m_pSecondIntroduce = new ThemedLable(this);
    m_pSecondIntroduce->setPaletteColor(QPalette::Text, DPalette::BrightText, 0.4);
    m_pSecondIntroduce->setFixedWidth(300);
    m_pSecondIntroduce->setAlignment(Qt::AlignCenter);
    m_pSecondIntroduce->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pSecondIntroduce, DFontSizeManager::T5, QFont::Normal);

    auto secondIntrlabelLayout = new QHBoxLayout();
    secondIntrlabelLayout->setContentsMargins(20, 0, 20, 0);
    secondIntrlabelLayout->addStretch();
    secondIntrlabelLayout->addWidget(m_pSecondIntroduce, 0, Qt::AlignCenter);
    secondIntrlabelLayout->addStretch();

    auto introduceLayout = new QVBoxLayout();
    introduceLayout->setContentsMargins(0, 10, 0, 30);
    introduceLayout->setSpacing(0);
    introduceLayout->addLayout(firstIntrlabelLayout);
    introduceLayout->addSpacing(10);
    introduceLayout->addLayout(secondIntrlabelLayout);

    m_pSpinner = new DSpinner(this);
    m_pSpinner->setFixedSize(14, 14);

    m_pLoadingLabel = new ThemedLable(this);
    m_pLoadingLabel->setPaletteColor(QPalette::Text, DPalette::BrightText, 0.4);
    m_pLoadingLabel->setAlignment(Qt::AlignCenter);
    m_pLoadingLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pLoadingLabel, DFontSizeManager::T6, QFont::Normal);

    m_pLoadingWidget = new DWidget(this);
    auto loadingLayout = new QHBoxLayout(m_pLoadingWidget);
    loadingLayout->setContentsMargins(0, 10, 0, 35);
    loadingLayout->setSpacing(4);
    loadingLayout->addStretch();
    loadingLayout->addWidget(m_pSpinner, 0, Qt::AlignRight);
    loadingLayout->addWidget(m_pLoadingLabel, 0, Qt::AlignLeft);
    loadingLayout->addStretch();

    m_pFreeAccount = new DSuggestButton(this);
    m_pFreeAccount->setFixedSize(224, 36);
    DFontSizeManager::instance()->bind(m_pFreeAccount, DFontSizeManager::T6, QFont::Normal);

    m_pSpacer = new QSpacerItem(1, 30, QSizePolicy::Fixed, QSizePolicy::Minimum);

    m_pExplainLabel = new ThemedLable(tr("After claiming, the originally gifted models will be replaced by the \"Intelligent Routing\" model"), this);
    m_pExplainLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle, 0.4);
    m_pExplainLabel->setFixedWidth(320);
    m_pExplainLabel->setAlignment(Qt::AlignCenter);
    m_pExplainLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pExplainLabel, DFontSizeManager::T6, QFont::Normal);

    auto explainlabelLayout = new QHBoxLayout();
    explainlabelLayout->setContentsMargins(0, 0, 0, 0);
    explainlabelLayout->addStretch();
    explainlabelLayout->addWidget(m_pExplainLabel, 0, Qt::AlignCenter);
    explainlabelLayout->addStretch();

    m_pLaterLabel = new DCommandLinkButton(tr("Manually collect later"), this);
    m_pLaterLabel->setContentsMargins(0, 0, 0, 0);
    m_pLaterLabel->setFixedWidth(320);
    DFontSizeManager::instance()->bind(m_pLaterLabel, DFontSizeManager::T6, QFont::Normal);

    auto activityLabelLayout = new QVBoxLayout();
    activityLabelLayout->setContentsMargins(0, 0, 0, 0);
    activityLabelLayout->setSpacing(0);
    activityLabelLayout->addLayout(explainlabelLayout);
    activityLabelLayout->addWidget(m_pLaterLabel, 0, Qt::AlignCenter);

    m_pFreeWidget = new DWidget(this);
    auto freeLayout = new QVBoxLayout(m_pFreeWidget);
    freeLayout->setContentsMargins(0, 4, 0, 20);
    freeLayout->setSpacing(10);
    freeLayout->addWidget(m_pFreeAccount, 0, Qt::AlignCenter);
    freeLayout->addLayout(activityLabelLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    mainLayout->addLayout(introduceLayout);
    mainLayout->addWidget(m_pLoadingWidget, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_pFreeWidget, 0, Qt::AlignHCenter);

    this->setLayout(mainLayout);
}

void DeepSeekInfo::initConnect()
{
    connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::getAccount);
    connect(m_pLaterLabel, &DSuggestButton::clicked, this, &DeepSeekInfo::close);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
}

void DeepSeekInfo::onUpdateSystemFont(const QFont &font)
{
    qCDebug(logAIGUI) << "Updating system font, new pixel size:" << QFontInfo(font).pixelSize();
    QFont tmpFont = m_pFirstIntroduce->font();
    tmpFont.setPixelSize(QFontInfo(font).pixelSize() + 8);
    m_pFirstIntroduce->setFont(tmpFont);
    this->adjustSize();
}

void DeepSeekInfo::checkActivity()
{
    qCInfo(logAIGUI) << "Starting to check DeepSeek activity status";
    bool dpsk = false;
    for (auto tmp : DbWrapper::localDbWrapper().queryLlmList()) {
        if (tmp.type == FREE_NORMAL) {
            if (tmp.model == UOS_FREE) {
                dpsk = true;
                qCDebug(logAIGUI) << "UOS_FREE model already exists:" << tmp.id;
            } else if (tmp.model == DeepSeek_Uos_Free
                       || (tmp.model >= SPARKDESK && tmp.model <= WXQF_ERNIE_Bot_4)) {
                // 兼容升级：旧免费模型（SPARKDESK/ERNIE/DeepSeek_Uos_Free）均通过
                // getAccount() 流程迁移到 UOS_FREE
                enrie = tmp;
                qCDebug(logAIGUI) << "Found old free model, pending migration:" << tmp.id << tmp.model;
            }
        }
    }

    if (dpsk) {
        qCInfo(logAIGUI) << "DeepSeek model already exists, skipping activity check";
        return;
    }
    
    adjustReceiveBtn(ReceivedStatus::CHECKING);
#ifdef ENABLE_FREEACCOUNT
    {
        qCInfo(logAIGUI) << "Free account enabled, starting activity check";
#else
        if (UOSAI_NAMESPACE::Util::checkLanguage()) {
            qCInfo(logAIGUI) << "Language check passed, starting activity check";
#endif
            QFuture<int> future = QtConcurrent::run([ = ] {
                qCDebug(logAIGUI) << "Starting background activity check";
                UosFreeAccountActivity hasActivity;
                auto error = UosFreeAccounts::instance().freeAccountButtonDisplay("account", hasActivity);
                if (error != QNetworkReply::NoError) {
                    qCWarning(logAIGUI) << "Free account activity check network error:" << error;
                    return (int)error;
                }

                qCDebug(logAIGUI) << "Activity check result, display flag:" << hasActivity.display;
                return hasActivity.display != 0 ? 0 : -1;
            });

            watcher.setFuture(future);
            connect(&watcher, &QFutureWatcher<int>::finished, this, &DeepSeekInfo::onCheckActivity);
        }
}

bool DeepSeekInfo::checkAndShow()
{
    qCInfo(logAIGUI) << "Checking if DeepSeek info dialog should be shown";

    if (QThread::currentThread() != qApp->thread()) {
        Q_ASSERT_X(QThread::currentThread() == qApp->thread(), __func__, "invalid working thread");
        qCCritical(logAIGUI) << "DeepSeekInfo dialog processing is not called in GUI thread";
        return false;
    }

    if (!needGuide()) {
        qCInfo(logAIGUI) << "Guide not needed, skipping dialog display";
        return true;
    }

    if (isDialogOpen) {
        getInstance().activateWindow();
        return false;
    }

    qCInfo(logAIGUI) << "Showing DeepSeek info dialog";
    isDialogOpen = true;
    getInstance().checkActivity();
    getInstance().adjustSize();
    getInstance().setDisplayPosition(DisplayPosition::Center);
    getInstance().exec();

    DbWrapper::localDbWrapper().updateGuideKey(QString::number(DbWrapper::builtinGuideVersion()));
    return true;
}

void DeepSeekInfo::migrateOldFreeModelAsync()
{
    LLMServerProxy oldModel;

    for (auto tmp : DbWrapper::localDbWrapper().queryLlmList()) {
        if (tmp.type == FREE_NORMAL) {
            if (tmp.model == UOS_FREE)
                return; // 已有新模型，无需迁移
            else {
                oldModel = tmp;
            }
        }
    }

    if (oldModel.id.isEmpty())
        return;

    qCInfo(logAIGUI) << "Old free model detected, starting silent migration:" << oldModel.id << oldModel.model;

    QtConcurrent::run([oldModel]() {
        UosFreeAccount freeAccount;
        int status = -1;
        auto error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, UOS_FREE, freeAccount, status);

        if (error != QNetworkReply::NoError || freeAccount.llmModel != UOS_FREE) {
            qCWarning(logAIGUI) << "Silent migration failed, will retry on next launch. error:" << error;
            return;
        }

        LLMServerProxy updated = oldModel;
        updated.model = UOS_FREE;
        updated.name = freeLlmName();
        updated.account.apiKey = freeAccount.appkey;

        DbWrapper::localDbWrapper().updateLlm(updated);

        qCInfo(logAIGUI) << "Silent migration to UOS_FREE completed successfully";
    });
}

bool DeepSeekInfo::needGuide()
{
    auto cur = DbWrapper::localDbWrapper().getGuideKey();
    bool needsGuide = cur.isEmpty() || cur.toInt() < DbWrapper::builtinGuideVersion();
    qCInfo(logAIGUI) << "Guide needed check - current:" << cur << "builtin:" << DbWrapper::builtinGuideVersion() << "needs guide:" << needsGuide;
    return needsGuide;
}

void DeepSeekInfo::getAccount()
{
    qCInfo(logAIGUI) << "Starting to get DeepSeek free account";
    adjustReceiveBtn(ReceivedStatus::RECEIVING);
    {
        QFuture<int> future = QtConcurrent::run([this] {
            qCDebug(logAIGUI) << "Background thread: Getting free account";
            UosFreeAccount freeAccount;

            int status = -1;
            auto error = UosFreeAccounts::instance().getFreeAccount(ModelType::FREE_NORMAL, UOS_FREE, freeAccount, status);
            if (status == 1) { // 活动结束
                qCWarning(logAIGUI) << "Free account activity has ended";
                return 1;
            }
            if (error != QNetworkReply::NoError) { // 请求错误，提示稍后再试
                qCWarning(logAIGUI) << "Network error while getting free account:" << error;
                return -1;
            }
            if (freeAccount.llmModel != UOS_FREE) {
                qCCritical(logAIGUI) << "getFreeAccount error: return model type" << freeAccount.llmModel << "need" << UOS_FREE;
                return 2;
            }
            
            qCInfo(logAIGUI) << "Free account obtained successfully, app key:" << freeAccount.appkey.left(8) + "...";
            
            LLMServerProxy tmp;
            tmp.id = freeAccount.appkey;
            tmp.type = FREE_NORMAL;
            tmp.name = freeLlmName();
            tmp.account.apiKey = freeAccount.appkey;
            tmp.model = UOS_FREE;

            if (!enrie.id.isEmpty()) {
                qCInfo(logAIGUI) << "Removing old Ernie model:" << enrie.id;
                DbWrapper::localDbWrapper().deleteLlm(enrie.id);
            }

            qCInfo(logAIGUI) << "Adding new DeepSeek model to database";
            DbWrapper::localDbWrapper().appendLlm(tmp);

            QList<AssistantProxy> assistantLst = DbWrapper::localDbWrapper().queryAssistantList();
            for (const AssistantProxy &assis : assistantLst) {
                if (assis.type == AssistantType::UOS_AI) {
                    qCInfo(logAIGUI) << "Updating UOS AI assistant to use DeepSeek model";
                    DbWrapper::localDbWrapper().updateAssistantLlm(assis.id, tmp.id);
                    break;
                }
            }

            DbWrapper::localDbWrapper().updateGuideKey(QString::number(DbWrapper::builtinGuideVersion()));
            qCInfo(logAIGUI) << "DeepSeek account setup completed successfully";
            return 0;
        });

        watcher.setFuture(future);
        connect(&watcher, &QFutureWatcher<int>::finished, this, &DeepSeekInfo::onGetAccount);
    }
}

void DeepSeekInfo::onCheckActivity()
{
    disconnect(&watcher, nullptr, this, nullptr);
    int result = watcher.result();
    qCInfo(logAIGUI) << "Activity check completed with result:" << result;

    if (result == 0 ) {
        qCInfo(logAIGUI) << "Activity exists, showing get account button";
        adjustReceiveBtn(ReceivedStatus::ACTIVITY_EXIST);
    } else if (result == -1) {
        qCWarning(logAIGUI) << "Activity has ended";
        adjustReceiveBtn(ReceivedStatus::ACTIVITY_OVER);
    } else {
        qCWarning(logAIGUI) << "Activity check failed";
        adjustReceiveBtn(ReceivedStatus::CHECK_FAIL);
    }
}

void DeepSeekInfo::closeEvent(QCloseEvent *event)
{
    isDialogOpen = false;
    DAbstractDialog::closeEvent(event);
}

void DeepSeekInfo::onGetAccount()
{
    disconnect(&watcher, nullptr, this, nullptr);
    int result = watcher.result();
    qCInfo(logAIGUI) << "Get account operation completed with result:" << result;

    if (result == 0) {
        qCInfo(logAIGUI) << "Account received successfully";
        adjustReceiveBtn(ReceivedStatus::RECEIVE_SUCCEED);
    } else if (result == 1) {
        qCWarning(logAIGUI) << "Activity has ended during account retrieval";
        adjustReceiveBtn(ReceivedStatus::ACTIVITY_OVER);
    } else {
        qCWarning(logAIGUI) << "Failed to receive account";
        adjustReceiveBtn(ReceivedStatus::RECEIVE_FAIL);
    }
}

void DeepSeekInfo::adjustReceiveBtn(int type)
{
    ReceivedStatus status = static_cast<ReceivedStatus>(type);
    disconnect(m_pFreeAccount, nullptr, this, nullptr);

    hideAllComponents();
    changeIntroduceText(type);
    switch (status)
    {
        case ReceivedStatus::ACTIVITY_EXIST:
            m_pFreeAccount->setText(tr("Get a free account"));
            m_pFreeAccount->show();
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pExplainLabel->show();
            m_pFreeWidget->show();
            connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::getAccount);
            break;
        case ReceivedStatus::CHECK_FAIL:
            m_pFreeAccount->setText(tr("Try again"));
            m_pFreeAccount->show();
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pFreeWidget->show();
            connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::checkActivity);
            break;
        case ReceivedStatus::ACTIVITY_OVER:
            m_pFreeAccount->setText(tr("Enter UOS AI"));
            m_pFreeAccount->show();
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pFreeWidget->show();
            connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::close);
            break;
        case ReceivedStatus::RECEIVING:
            m_pFreeAccount->setText(tr("Receiving..."));
            m_pFreeAccount->show();
            m_pFreeAccount->setEnabled(false);
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pExplainLabel->show();
            m_pFreeWidget->show();
            break;
        case ReceivedStatus::RECEIVE_SUCCEED:
            m_pFreeAccount->setText(tr("Use it immediately"));
            m_pFreeAccount->show();
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pFreeWidget->show();
            connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::close);
            break;
        case ReceivedStatus::RECEIVE_FAIL:
            m_pFreeAccount->setText(tr("Try again"));
            m_pFreeAccount->show();
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pLaterLabel->show();
            m_pFreeWidget->show();
            connect(m_pFreeAccount, &DSuggestButton::clicked, this, &DeepSeekInfo::getAccount);
            break;
        case ReceivedStatus::CHECKING:
            m_pFirstIntroduce->show();
            m_pSecondIntroduce->show();
            m_pSpinner->start();
            m_pSpinner->show();
            m_pLoadingLabel->setText(tr("Checking account status..."));
            m_pLoadingLabel->show();
            m_pLoadingWidget->show();
            break;
        default:
            break;

    }
    QTimer::singleShot(50, this, &DeepSeekInfo::adjustSize);
}

void DeepSeekInfo::hideAllComponents()
{
    m_pFreeAccount->hide();
    m_pFreeAccount->setEnabled(true);
    m_pFirstIntroduce->hide();
    m_pSecondIntroduce->hide();
    m_pSpinner->stop();
    m_pSpinner->hide();
    m_pLoadingLabel->hide();
    m_pExplainLabel->hide();
    m_pLaterLabel->hide();
    m_pLoadingWidget->hide();
    m_pFreeWidget->hide();
}

void DeepSeekInfo::changeIntroduceText(int type)
{
    ReceivedStatus status = static_cast<ReceivedStatus>(type);

    QVBoxLayout *freeLayout = qobject_cast<QVBoxLayout*>(m_pFreeWidget->layout());

    if (status == ReceivedStatus::ACTIVITY_OVER) {
        m_pFirstIntroduce->setText(tr("UOS AI has fully integrated the \"Intelligent Routing\" model capability"));
        m_pSecondIntroduce->setText(tr("The free account activity has ended."));
        freeLayout->addSpacerItem(m_pSpacer);
    } else if (status == ReceivedStatus::CHECK_FAIL) {
        m_pFirstIntroduce->setText(tr("Failed to claim the model quota!"));
        m_pSecondIntroduce->setText(tr("Please check the network and try again later！"));
        freeLayout->addSpacerItem(m_pSpacer);
    } else if (status == ReceivedStatus::RECEIVE_SUCCEED) {
        m_pFirstIntroduce->setText(tr("Model quota successfully claimed!"));
        m_pSecondIntroduce->setText(tr("Come and experience it!"));
        freeLayout->addSpacerItem(m_pSpacer);
    } else if (status == ReceivedStatus::RECEIVE_FAIL) {
        m_pFirstIntroduce->setText(tr("Failed to claim the model quota!"));
        m_pSecondIntroduce->setText(tr("Please check the network and try again or manually claim in UOS AI settings later"));
        freeLayout->removeItem(m_pSpacer);
    } else {
        m_pFirstIntroduce->setText(tr("UOS AI has fully integrated the \"Intelligent Routing\" model capability"));
        m_pSecondIntroduce->setText(tr("Come and claim your account!"));
        freeLayout->removeItem(m_pSpacer);
    }
}
