#include "mcpserveritem.h"
#include <localmodelserver.h>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLoggingCategory>
#include <QtConcurrent>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

McpServerItem::McpServerItem(DWidget *parent)
    : DWidget(parent)
    , m_pProcess(new QProcess(this))
{
    initUI();
    initConnect();
}

McpServerItem::~McpServerItem()
{
    if (m_pProcess)
        m_pProcess->terminate();
}

void McpServerItem::initUI()
{
    // 左侧信息区域
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);

    m_pNameLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pNameLabel, DFontSizeManager::T6, QFont::Medium);

    m_pDescLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_pDescLabel, DFontSizeManager::T8, QFont::Normal);
    m_pDescLabel->setElideMode(Qt::ElideRight);

    infoLayout->addWidget(m_pNameLabel);
    infoLayout->addWidget(m_pDescLabel);

    // 安装相关按钮
    m_pBtnInstall = new DSuggestButton(tr("Install"));
    m_pBtnInstall->setFixedHeight(30);
    m_pBtnInstall->setMinimumWidth(70);
    m_pBtnInstall->setMaximumWidth(80);

    m_pBtnUninstall = new DPushButton(tr("Uninstall"));
    m_pBtnUninstall->setFixedHeight(30);
    m_pBtnUninstall->setMinimumWidth(70);
    m_pBtnUninstall->setMaximumWidth(100);
    m_pBtnUninstall->hide();

    // 更新按钮
    m_pBtnUpdate = new DSuggestButton(tr("Update"));
    m_pBtnUpdate->setFixedHeight(30);
    m_pBtnUpdate->setMinimumWidth(70);
    m_pBtnUpdate->setMaximumWidth(90);
    m_pBtnUpdate->hide();

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(infoLayout, 10);
    mainLayout->setSpacing(10);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pBtnInstall, 2, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUninstall, 2, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUpdate, 2, Qt::AlignVCenter);
}

void McpServerItem::initConnect()
{
    connect(m_pBtnInstall, &DSuggestButton::clicked, this, &McpServerItem::onInstall);
    connect(m_pBtnUninstall, &DPushButton::clicked, this, &McpServerItem::onUninstall);
    connect(m_pBtnUpdate, &DSuggestButton::clicked, this, &McpServerItem::onUpdate);
}

void McpServerItem::setText(const QString &theme, const QString &summary)
{
    m_pNameLabel->setText(theme);
    m_pDescLabel->setText(summary);
    m_pDescLabel->setToolTip(summary);
}

void McpServerItem::setAppName(const QString &appName)
{
    m_appName = appName;
}

void McpServerItem::onInstall()
{
    qCDebug(logAIGUI) << "Install MCP server clicked. AppName:" << m_appName;

    if (m_appName.isEmpty())
        return;

    // 调用应用商店
    LocalModelServer::getInstance().openInstallWidget(m_appName);

    Q_EMIT doCheckInstalled();
}

void McpServerItem::onUninstall()
{
    qCDebug(logAIGUI) << "Uninstall MCP server clicked. AppName:" << m_appName;

    if (m_appName.isEmpty()) {
        qCWarning(logAIGUI) << "App name is empty!!!";
        return;
    }

    // 调用应用商店
    LocalModelServer::getInstance().openManagerWidget();
    Q_EMIT doCheckInstalled();
}

void McpServerItem::onUpdate()
{
    qCDebug(logAIGUI) << "Update button clicked. AppName:" << m_appName;
    if (m_appName.isEmpty())
        return;
    // 可直接调用安装逻辑
    LocalModelServer::getInstance().openInstallWidget(m_appName);
    Q_EMIT doCheckInstalled();
}

void McpServerItem::checkUpdateStatus(bool isInstalled)
{
    if (!isInstalled) {
        m_pBtnUpdate->hide();
        adjustSummaryLabelWidth();
        return;
    }

    QFuture<void> future = QtConcurrent::run([=]() {
        QProcess process;
        process.start("apt", QStringList() << "list" << "--upgradable");
        process.waitForFinished();
        QByteArray reply = process.readAllStandardOutput();
        QString updateVersion = getUpdateVersion(reply);
        QMetaObject::invokeMethod(this, [this, updateVersion]() {
            if (!updateVersion.isEmpty()) {
                m_updateVersion = updateVersion;
                m_pBtnUpdate->show();
            } else {
                m_updateVersion.clear();
                m_pBtnUpdate->hide();
            }
            adjustSummaryLabelWidth();
            //qCInfo(logAIGUI) << "AppName:" << m_appName << "upgradable version:" << m_updateVersion;
        }, Qt::QueuedConnection);
        process.deleteLater();
    });
}

QString McpServerItem::getUpdateVersion(const QByteArray& reply)
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

void McpServerItem::adjustSummaryLabelWidth()
{
    m_pBtnInstall->adjustSize();
    m_pBtnUpdate->adjustSize();
    m_pBtnUninstall->adjustSize();

    int maxWidth = this->size().width() - 40;
    if (m_pBtnInstall->isVisible())
        maxWidth -= m_pBtnInstall->width();
    if (m_pBtnUninstall->isVisible())
        maxWidth -= m_pBtnUninstall->width();
    if (m_pBtnUpdate->isVisible())
        maxWidth -= m_pBtnUpdate->width();

    m_pDescLabel->setMaximumWidth(maxWidth < 0 ? 0 : maxWidth);
}

void McpServerItem::changeInstallStatus(bool isInstalled)
{
    if (isInstalled) {
        m_pBtnInstall->hide();
        m_pBtnUninstall->show();
    } else {
        m_pBtnUninstall->hide();
        m_pBtnInstall->show();
    }

    checkUpdateStatus(isInstalled);
    adjustSummaryLabelWidth();
}

void McpServerItem::resizeEvent(QResizeEvent *event)
{
    DWidget::resizeEvent(event);
    adjustSummaryLabelWidth();
}
