#include "localmodelitem.h"
#include "localmodelserver.h"
#include "dbwrapper.h"
#include "dconfigmanager.h"

#include <DFontSizeManager>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <QResizeEvent>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr int TIMERBEGIN = 720;//5秒轮询安装和更新状态60分钟

LocalModelItem::LocalModelItem(DWidget *parent)
    : DWidget(parent)
    , m_pProcess(new QProcess)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    initUI();
    initConnect();
}

LocalModelItem::~LocalModelItem()
{
    if (m_pProcess) {
        m_pProcess->terminate();
        m_pProcess->deleteLater();
    }
    m_timer->stop();
}

void LocalModelItem::initUI()
{
    m_pLabelTheme = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelTheme, DFontSizeManager::T6, QFont::Medium);
    m_pLabelSummary = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelSummary, DFontSizeManager::T8, QFont::Normal);
    m_pLabelSummary->setElideMode(Qt::ElideRight);

    m_redPoint = new DLabel(this);
    m_redPoint->setPixmap(QIcon(":/assets/images/redpoint.svg").pixmap(18, 18));
    m_redPoint->hide();

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setSpacing(5);
    titleLayout->addWidget(m_pLabelTheme);
    titleLayout->addWidget(m_redPoint);
    titleLayout->addStretch();

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addLayout(titleLayout);
    textLayout->addWidget(m_pLabelSummary);

    m_pBtnInstall = new DSuggestButton(tr("Install"));
    m_pBtnInstall->setFixedHeight(30);
    m_pBtnInstall->setMinimumWidth(70);
    m_pBtnInstall->setMaximumWidth(80);

    m_pBtnUninstall = new DPushButton(tr("Uninstall"));
    m_pBtnUninstall->setFixedHeight(30);
    m_pBtnUninstall->setMinimumWidth(70);
    m_pBtnUninstall->setMaximumWidth(100);
    m_pBtnUninstall->hide();

    m_pBtnUpdate = new DSuggestButton(tr("Update"));
    m_pBtnUpdate->setFixedHeight(30);
    m_pBtnUpdate->setMinimumWidth(70);
    m_pBtnUpdate->setMaximumWidth(90);
    m_pBtnUpdate->hide();

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(textLayout);
    mainLayout->setSpacing(10);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pBtnInstall, 0, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUninstall, 0, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUpdate, 0, Qt::AlignVCenter);

    setLayout(mainLayout);
}

void LocalModelItem::initConnect()
{
    connect(m_pBtnInstall, &DSuggestButton::clicked, this, &LocalModelItem::onInstall);
    connect(m_pBtnUpdate, &DSuggestButton::clicked, this, &LocalModelItem::onUpdate);
    connect(m_pBtnUninstall, &DSuggestButton::clicked, this, &LocalModelItem::onUninstall);
    connect(this,&LocalModelItem::changeUpdateStatus, this,&LocalModelItem::onChangeUpdateStatus);
    connect(m_timer, &QTimer::timeout, this, &LocalModelItem::checkStatusOntime);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::sigToLaunchTimer, this, &LocalModelItem::beginTimer);
}

void LocalModelItem::setText(const QString &theme, const QString &summary)
{
    m_pLabelTheme->setText(theme);
    m_pLabelSummary->setText(summary);
    m_pLabelSummary->setToolTip(summary);
}

void LocalModelItem::setAppName(const QString &appName)
{
    m_appName = appName;
}

void LocalModelItem::onInstall()
{
    qCInfo(logAIGUI) << "Install button clicked. AppName:" << m_appName;
    if (m_appName.isEmpty())
        return;
//    m_pBtnInstall->setText(tr("Installing"));
    LocalModelServer::getInstance().openInstallWidget(m_appName);
    beginTimer(TIMERBEGIN);
}

void LocalModelItem::onUpdate()
{
    qCInfo(logAIGUI) << "Update button clicked. AppName:" << m_appName;
    if (m_appName.isEmpty())
        return;
    m_redPoint->setVisible(false);
    saveUpdateVersion();
//    m_pBtnUpdate->setText(tr("Updating"));
    LocalModelServer::getInstance().openInstallWidget(m_appName);
    beginTimer(TIMERBEGIN);
}

void LocalModelItem::onUninstall()
{
    qCInfo(logAIGUI) << "Uninstall button clicked. AppName:" << m_appName;
    if (m_appName.isEmpty())
        return;
//    m_pBtnUninstall->setText(tr("Uninstalling"));
    LocalModelServer::getInstance().openManagerWidget();
    beginTimer(TIMERBEGIN);
}

void LocalModelItem::checkInstallStatus()
{
    if (!m_pProcess->atEnd()) return;
    if (m_pProcess->state() == QProcess::Running)
        m_pProcess->waitForFinished();
    m_pProcess->start("dpkg-query", QStringList() << "-W" << QString("-f='${db:Status-Status}\n'") << m_appName);
    m_pProcess->waitForFinished();
    QByteArray reply = m_pProcess->readAllStandardOutput();
    bool newInstallStatus = (reply == "'installed\n'" ? true : false);
    if (m_isInstall != newInstallStatus) {
        qCInfo(logAIGUI) << "Install status changed. AppName:" << m_appName << ", Installed:" << newInstallStatus;
        m_isInstall = newInstallStatus;
        LocalModelServer::getInstance().localModelStatusChanged(m_appName, m_isInstall);
    }
    changeInstallStatus();
}

void LocalModelItem::checkUpdateStatus()
{
    QFuture<void> future = QtConcurrent::run([=]() {
        if (!m_isInstall) {
            emit changeUpdateStatus(false);
            return;
        }
        QProcess process;
        process.start("apt", QStringList() << "list" << "--upgradable");
        process.waitForFinished();
        QByteArray reply = process.readAllStandardOutput();
        m_updateVersion = getUpdateVersion(reply);

        qCInfo(logAIGUI) << m_appName << "query update version" << m_updateVersion;
        if (!m_updateVersion.isEmpty()) {
            QString version = "";
            if (PLUGINSNAME == m_appName)
                version = DConfigManager::instance()->value(LLM_GROUP, LLM_UOSAIRAG).toString();
            bool isShowRedPoint = (m_updateVersion != version);
            qCInfo(logAIGUI) << m_appName << "show red point" << isShowRedPoint << version;
            m_redPoint->setVisible(isShowRedPoint);
            emit sigRedPointVisible(isShowRedPoint);
        }
        emit changeUpdateStatus(!m_updateVersion.isEmpty());
        process.deleteLater();
    });
}

QString LocalModelItem::getUpdateVersion(const QByteArray& reply)
{
    QString output = QString::fromUtf8(reply);

    QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        if (line.contains(m_appName)) {
            QStringList parts = line.split('/');
            if (parts.size() > 1) {
                QString versionPart = parts[1];
                QStringList versionInfo = versionPart.split(' ');
                if (versionInfo.size() > 1) {
                    return versionInfo[1].trimmed();
                }
            }
        }
    }
    return "";
}

void LocalModelItem::changeInstallStatus()
{
    if (m_appName.isEmpty()) return;
    if (m_isInstall) {
        m_pBtnInstall->hide();
        m_pBtnUninstall->show();
    } else {
        m_pBtnUninstall->hide();
        m_pBtnInstall->show();
    }
    adjustSummaryLabelWidth();
}

void LocalModelItem::onChangeUpdateStatus(bool isExistUpdate)
{
    qCDebug(logAIGUI) << "Change update status for LocalModelItem. AppName:" << m_appName << ", ExistUpdate:" << isExistUpdate;
    if (isExistUpdate)
        m_pBtnUpdate->show();
    else
        m_pBtnUpdate->hide();
    adjustSummaryLabelWidth();
}

void LocalModelItem::checkStatusOntime()
{
//    qInfo() << "checking LocalModel status, Time remaining " << m_timerCount * 3;
    if (m_timerCount > 0) {
        checkInstallStatus();
        checkUpdateStatus();
        --m_timerCount;
    } else {
        m_timerCount = 0;
        m_timer->stop();
    }
}

void LocalModelItem::beginTimer(const int &time)
{
    qCDebug(logAIGUI) << "Begin timer for LocalModelItem. Time:" << time;
    m_timerCount = time;
    m_timer->start();
}

void LocalModelItem::stopTimer()
{
    qCDebug(logAIGUI) << "Stop timer for LocalModelItem.";
    m_timerCount = 0;
    m_timer->stop();
}

void LocalModelItem::addLocalLlM()
{
    qCDebug(logAIGUI) << "addLocalLlM called.";
    //DbWrapper::appAssistant(); initAssistant();
    //DbWrapper::appendLlm();
}

bool LocalModelItem::getInstallStatus()
{
    return m_isInstall;
}

void LocalModelItem::saveUpdateVersion()
{
    qCDebug(logAIGUI) << "Saving update version for LocalModelItem. Version:" << m_updateVersion;
    if (!m_updateVersion.isEmpty())
        DConfigManager::instance()->setValue(LLM_GROUP, LLM_UOSAIRAG, m_updateVersion);
}

void LocalModelItem::resizeEvent(QResizeEvent *event)
{
    DWidget::resizeEvent(event);
    adjustSummaryLabelWidth();
}

void LocalModelItem::adjustSummaryLabelWidth()
{
    m_pBtnInstall->adjustSize();
    m_pBtnUpdate->adjustSize();
    m_pBtnUninstall->adjustSize();
    int maxWidth = this->size().width() - 40;
    if (m_isInstall)
        maxWidth -= m_pBtnUninstall->width();
    else
        maxWidth -= m_pBtnInstall->width();
    if (m_pBtnUpdate->isVisible())
        maxWidth -= m_pBtnUpdate->width();
    m_pLabelSummary->setMaximumWidth(maxWidth < 0 ? 0 : maxWidth);
}


