// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "modelscopeitem.h"
#include "downloader.h"
#include "modelupdater.h"
#include "localmodelserver.h"
#include "dbwrapper.h"
#include "externalllm/modelhubwrapper.h"
#include "downloader.h"
#include "serverwrapper.h"

#include <DFontSizeManager>
#include <DDialog>
#include <DDesktopServices>
#include <DFontSizeManager>
#include <DDialog>

#include <QStandardPaths>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <QResizeEvent>
#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

ModelScopeItem::ModelScopeItem(const ItemInfo &info, DWidget *parent)
    : m_info(info)
    , DWidget(parent)
{
    initUI();
    initConnect();

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    m_installPath = homePath + "/.local/share/deepin-modelhub/models";
}

ModelScopeItem::~ModelScopeItem()
{
}

void ModelScopeItem::initUI()
{
    m_pLabelTheme = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelTheme, DFontSizeManager::T6, QFont::Medium);
    m_pLabelSize = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelSize, DFontSizeManager::T8, QFont::Normal);
    m_pLabelSize->setVisible(false);

    m_pSpinner = new DSpinner;
    m_pSpinner->setFixedSize(QSize(18, 18));
    m_pSpinner->setVisible(false);
    m_pLabelSummary = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelSummary, DFontSizeManager::T8, QFont::Normal);
    m_pLabelSummary->setElideMode(Qt::ElideRight);

    m_redPoint = new DLabel(this);
    m_redPoint->setScaledContents(true);
    m_redPoint->setPixmap(QIcon(":/assets/images/redpoint.svg").pixmap(QSize(128, 128)));
    m_redPoint->setFixedSize(16, 16);
    m_redPoint->hide();

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setSpacing(4);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->addWidget(m_pLabelTheme);
    titleLayout->addWidget(m_pLabelSize);

    QVBoxLayout *redPointLayout = new QVBoxLayout;
    redPointLayout->setContentsMargins(0, 0, 0, 0);
    redPointLayout->setSpacing(0);
    redPointLayout->addStretch(1);
    redPointLayout->addWidget(m_redPoint);
    redPointLayout->addStretch(1);
    
    titleLayout->addLayout(redPointLayout);
    titleLayout->addStretch();

    QHBoxLayout *summaryLayout = new QHBoxLayout;
    summaryLayout->addWidget(m_pSpinner);
    summaryLayout->addWidget(m_pLabelSummary);
    summaryLayout->addStretch();

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addLayout(titleLayout);
    textLayout->addLayout(summaryLayout);

    m_pBtnInstall = new DSuggestButton(tr("Install"), this);
    m_pBtnInstall->setFixedHeight(30);
    m_pBtnInstall->setMinimumWidth(70);

    m_pBtnUninstall = new DPushButton(tr("Uninstall"), this);
    m_pBtnUninstall->setFixedHeight(30);
    m_pBtnUninstall->setMinimumWidth(70);
    m_pBtnUninstall->hide();

    m_pBtnUpdate = new UpdateButton(this);
    m_pBtnUpdate->setText(tr("Update"));
    m_pBtnUpdate->setFixedHeight(30);
    m_pBtnUpdate->setMinimumWidth(70);
    m_pBtnUpdate->hide();

    m_pBtnStop = new DPushButton(tr("Cancel"), this);
    m_pBtnStop->setFixedHeight(30);
    m_pBtnStop->setMinimumWidth(70);
    m_pBtnStop->hide();

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(textLayout);
    mainLayout->setSpacing(10);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pBtnInstall, 0, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUninstall, 0, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnUpdate, 0, Qt::AlignVCenter);
    mainLayout->addWidget(m_pBtnStop, 0, Qt::AlignVCenter);

    setLayout(mainLayout);
}

void ModelScopeItem::initConnect()
{
    connect(m_pBtnInstall, &DSuggestButton::clicked, this, &ModelScopeItem::onInstall);
    connect(m_pBtnUninstall, &DSuggestButton::clicked, this, &ModelScopeItem::onUninstall);
    connect(m_pBtnStop, &DSuggestButton::clicked, this, &ModelScopeItem::stopDownload);
    connect(m_pBtnUpdate, &UpdateButton::sigButtonclicked, this, [ = ](bool status){
        if (status)
            onUpdate();
        else
            stopDownload();
    });
    connect(&LocalModelServer::getInstance(), &LocalModelServer::modelPluginsStatusChanged, this, &ModelScopeItem::checkInstallStatus);
}

void ModelScopeItem::setText(const QString &theme, const QString &summary, double sizeGb)
{
    m_pLabelTheme->setText(theme);
    m_pLabelSummary->setText(summary);
    m_pLabelSummary->setToolTip(summary);
    m_sizeGb = sizeGb;
    m_pLabelSize->setText(QString(tr("About %1GB")).arg(m_sizeGb));
}

void ModelScopeItem::startDownload()
{
    m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
    QDir destinationDir(m_installPath + "/." + m_info.appName);
    destinationDir.removeRecursively();
    if (destinationDir.mkpath("gguf")) {
        qCInfo(logAIGUI) << "Directory created successfully for download. appName:" << m_info.appName;
    } else {
        qCWarning(logAIGUI) << "Failed to create directory for download. appName:" << m_info.appName;
        return;
    }

    destinationDir.cd("gguf");

    m_downloader.reset(new Downloader(destinationDir.absolutePath()));
    connect(m_downloader.data(), &Downloader::downloadFinished, this, &ModelScopeItem::onCheckFile);
    connect(m_downloader.data(), &Downloader::onDownloadProgress, this, &ModelScopeItem::onDownloadProgress);

    foreach (const QString &fileUrl, m_info.modelFileList) {
        QString url = m_info.baseUrl + "/" + m_info.appName + "/resolve/master/" + fileUrl;
        m_downloader->addDownloadTask(QUrl(url));
    }

    if (m_timer == nullptr) {
        m_timer = new QTimer(this);
        m_timer->setInterval(30000);
        connect(m_timer, &QTimer::timeout, this, [&] {
            this->stopDownload();
            this->onCheckFileFailed();
            m_pBtnUpdate->hide();
            m_pBtnUpdate->setStatus(true);
            QTimer::singleShot(1, this, &ModelScopeItem::adjustSummaryLabelWidth);
        });
    }
    m_timer->start();
    qCInfo(logAIGUI) << "Start download for model. appName:" << m_info.appName;
}

void ModelScopeItem::onInstall()
{
    m_pBtnInstall->setText(tr("Installing"));
    m_pBtnInstall->setEnabled(false);
    m_pBtnInstall->setVisible(false);
    m_pLabelSize->setVisible(false);
    m_pBtnStop->show();
    qCInfo(logAIGUI) << "Install button clicked. appName:" << m_info.appName;
    startDownload();

    QTimer::singleShot(1, this, &ModelScopeItem::adjustSummaryLabelWidth);
}

void ModelScopeItem::onUpdate()
{
    m_pBtnInstall->setEnabled(false);
    m_redPoint->setVisible(false);
    m_updater->saveNewHash();
    qCInfo(logAIGUI) << "Update button clicked. appName:" << m_info.appName;
    startDownload();

    QTimer::singleShot(1, this, &ModelScopeItem::adjustSummaryLabelWidth);
}

void ModelScopeItem::onUninstall()
{
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMaximumWidth(380);
    dlg.setTitle(tr("Are you sure you want to delete this model?"));
    dlg.addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    qCInfo(logAIGUI) << "Uninstall button clicked. appName:" << m_info.appName;
    if (DDialog::Accepted == dlg.exec()) {
        QString modelName = m_installPath + "/" + m_info.modelName;
        QDir installFile(modelName);
        if (installFile.exists()) {
            if (installFile.removeRecursively())
                qCInfo(logAIGUI) << "Directory removed successfully. appName:" << m_info.appName;
            else
                qCWarning(logAIGUI) << "Failed to remove directory. appName:" << m_info.appName;
        }
        checkInstallStatus();
    } else {
        qCDebug(logAIGUI) << "Uninstall dialog canceled. appName:" << m_info.appName;
    }
}

void ModelScopeItem::onCheckFile()
{
    if (m_timer) {
        m_timer->stop();
    }
    m_pBtnInstall->setText(tr("Checking"));
    m_pBtnInstall->setVisible(true);
    m_pBtnStop->hide();
    m_pBtnUninstall->hide();
    m_pBtnUpdate->hide();
    m_pBtnUpdate->setStatus(true);
    m_pBtnInstall->adjustSize();
    m_pLabelSummary->setText(m_pLabelSummary->toolTip());
    m_pSpinner->stop();
    m_pSpinner->setVisible(false);
    qCDebug(logAIGUI) << "Checking downloaded files. appName:" << m_info.appName;
    QtConcurrent::run(&ModelScopeItem::runCheckFile, this, QString(m_installPath + "/." + m_info.appName), m_info.modelFileList);
}

void ModelScopeItem::onCheckFileFailed()
{
    checkInstallStatus();

    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.addButton(tr("Ok", "button"), true, DDialog::ButtonNormal);
    dlg.setTitle(tr("%0 download failed, please try again!").arg(m_pLabelTheme->text()));
    dlg.exec();

    QDir dir(m_installPath + "/." + m_info.appName);
    dir.removeRecursively();
    qCWarning(logAIGUI) << "Check file failed, download failed. appName:" << m_info.appName;
}

void ModelScopeItem::checkInstallStatus()
{
    bool lastStatus = m_isInstall;
    m_isInstall = ModelhubWrapper::isModelInstalled(m_info.modelName);

    if (lastStatus != m_isInstall)
        ServerWrapper::instance()->updateLLMAccount();

    //判断模型是否在用户路径
    if (m_isInstall) {
        QString modelPath = m_installPath + "/" + m_info.modelName;
        QDir modelDir(modelPath);
        m_pBtnUninstall->setEnabled(modelDir.exists());
        checkUpdateStatus();
    }

    QTimer::singleShot(1, this, &ModelScopeItem::changeInstallStatus);
    qCDebug(logAIGUI) << "Check install status. appName:" << m_info.appName << ", isInstall:" << m_isInstall;
}

void ModelScopeItem::changeInstallStatus()
{
    if (m_isInstall) {
        m_pBtnInstall->hide();
        m_pBtnUninstall->show();
        m_pBtnStop->hide();
    } else {
        m_pBtnUninstall->hide();
        m_pBtnUpdate->hide();
        m_pBtnInstall->show();
        m_pBtnStop->hide();
        m_pBtnInstall->setText(tr("Install"));
        m_pBtnInstall->setEnabled(true);
        m_pBtnInstall->setVisible(true);
        m_pLabelSize->setVisible(true);
        if (ModelhubWrapper::isModelhubInstalled()) {
            m_pBtnInstall->setToolTip("");
        } else {
            m_pBtnInstall->setEnabled(false);
            m_pBtnInstall->setToolTip(tr("Please install the \"Embedding Plugins\" first before installing this model."));
        }
    }

    if (m_downloader && !m_downloader->isFinished() && !m_pBtnUpdate->isVisible()) {
        m_pBtnInstall->setEnabled(false);
        m_pBtnInstall->setVisible(false);
        m_pLabelSize->setVisible(false);
        m_pBtnStop->show();
    }

    QTimer::singleShot(1, this, &ModelScopeItem::adjustSummaryLabelWidth);
    qCDebug(logAIGUI) << "Change install status. appName:" << m_info.appName << ", isInstall:" << m_isInstall;
}

bool ModelScopeItem::getInstallStatus()
{
    return m_isInstall;
}

void ModelScopeItem::checkUpdateStatus()
{
    if (m_updater.isNull()) {
        m_updater.reset(new ModelUpdater(m_info.baseUrl, m_installPath, m_info.appName, m_info.modelName));

        connect(m_updater.data(), &ModelUpdater::canUpdate, this, [this](QPair<bool, bool> canUpdate){
            m_pBtnUpdate->setVisible(canUpdate.first);
            m_redPoint->setVisible(canUpdate.second);
            QTimer::singleShot(1, this, &ModelScopeItem::adjustSummaryLabelWidth);
            emit sigRedPointVisible(canUpdate.second);
        });
    }

    m_updater->check();
}

void ModelScopeItem::saveNewHashFile()
{
    if (m_updater)
        m_updater->saveNewHash();
}

void ModelScopeItem::resizeEvent(QResizeEvent *event)
{
    DWidget::resizeEvent(event);
    adjustSummaryLabelWidth();
}

void ModelScopeItem::runCheckFile(ModelScopeItem *self, const QString &dirPath, const QStringList &fileList)
{
    QDir dir(dirPath);
    dir.cd("gguf");
    if (dir.exists()) {
        QFile shaFile(dir.filePath("sha256"));
        if (shaFile.open(QFile::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(shaFile.readAll());
            QVariantHash shaKV = doc.object().toVariantHash();
            int success = fileList.size() - 1;
            if (shaKV.size() == (fileList.size() - 1)) {
                for (QString item : fileList) {
                    if (item == "sha256")
                        continue;

                    QString itemTmp = item;
                    // bug-25/02/28 QString remove()方法在AMD和ARM行为不一致
                    bool ok = Downloader::checkSha256(dir.filePath(itemTmp.remove("modelhub/")), shaKV.value(item).toString());
                    if (!ok) {
                        qWarning() << item << "sha256 is not matched.";
                        break;
                    } else {
                        success--;
                    }
                }
                if (success == 0) {
                    QMetaObject::invokeMethod(self, "onDownloadFinished");
                    return;
                }
            } else {
                qWarning() << "sha256 file" << shaKV << "is not match file count" << fileList;
            }
        }
    }

    QMetaObject::invokeMethod(self, "onCheckFileFailed");
}

void ModelScopeItem::adjustSummaryLabelWidth()
{
    m_pBtnInstall->adjustSize();
    m_pBtnUninstall->adjustSize();
    m_pBtnUpdate->adjustSize();

    int maxWidth = this->size().width() - 40;
    if (m_isInstall)
        maxWidth -= m_pBtnUninstall->width();
    else
        maxWidth -= m_pBtnInstall->width();

    if (m_pBtnStop->isVisible()) {
        maxWidth += m_pBtnInstall->width();
        maxWidth -= m_pBtnStop->width();
    }

    if (m_pBtnUpdate->isVisible()) {
        maxWidth -= m_pBtnUpdate->width();
    }

    m_pLabelSummary->setMaximumWidth(maxWidth < 0 ? 0 : maxWidth);
    qCDebug(logAIGUI) << "Adjust summary label width. appName:" << m_info.appName << ", maxWidth:" << maxWidth;
}

void ModelScopeItem::stopDownload()
{
    if (m_timer) {
        m_timer->stop();
    }
    m_pLabelSummary->setText(m_pLabelSummary->toolTip());
    m_pSpinner->stop();
    m_pSpinner->setVisible(false);
    if (m_downloader && !m_downloader->isFinished()) {
        QString modelName = m_installPath + "/." + m_info.appName;

        QDir installFile(modelName);

        if (installFile.exists()) {
            if (installFile.removeRecursively())
                qCInfo(logAIGUI) << "Directory removed successfully (stop download). appName:" << m_info.appName;
            else
                qCWarning(logAIGUI) << "Failed to remove directory (stop download). appName:" << m_info.appName;
        }
        m_downloader->cancelDownloads();
        checkInstallStatus();
        qCInfo(logAIGUI) << "Stop download for model. appName:" << m_info.appName;
    }
}

void ModelScopeItem::onDownloadFinished()
{
    QString originalFolderPath = m_installPath + "/." + m_info.appName;
    QString targetFolderPath = m_installPath + "/" + m_info.modelName;

    QDir dir(originalFolderPath);
    if (dir.exists()) {
        //check file
        QStringList lostFiles;
        {
            QDir ldir(originalFolderPath + "/gguf");
            QStringList files;
            for(auto info : ldir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
                files << info.fileName();

            for (const QString &needFile : m_info.modelFileList) {
                QFileInfo info(needFile);
                if (files.contains(info.fileName()))
                    continue;
                lostFiles << info.fileName();
            }
        }

        if (lostFiles.isEmpty()) {
            if (QFileInfo::exists(targetFolderPath)) {
                bool trashSuccess = DDesktopServices::trash(targetFolderPath);
                if (!trashSuccess)
                    qCWarning(logAIGUI) << "Trash request failed. appName:" << m_info.appName;

                for (int i = 0; i < 4 && QFileInfo::exists(targetFolderPath); ++i) {
                    qCInfo(logAIGUI) << "the" << i + 1 << "time trash old model" << targetFolderPath;
                    QThread::msleep(500);
                }
            }

            if (dir.rename(originalFolderPath, targetFolderPath)) {
                qCInfo(logAIGUI) << "Folder renamed successfully." << targetFolderPath;
            } else {
                qCWarning(logAIGUI) << "Folder renaming failed." << originalFolderPath << targetFolderPath;
                if (QFileInfo::exists(targetFolderPath)) {
                    qCInfo(logAIGUI) << "target folder trash failed. appName:" << m_info.appName;
                    DDialog dlg(this);
                    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
                    dlg.addButton(tr("Ok", "button"), true, DDialog::ButtonNormal);
                    dlg.setTitle(tr("target folder trash failed, please try again!").arg(m_pLabelTheme->text()));
                    dlg.exec();
                }
            }
        } else {
            qCWarning(logAIGUI) << "fail to download" << lostFiles << ". appName:" << m_info.appName;
        }
    }
    checkInstallStatus();
}

void ModelScopeItem::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_downloader && !m_downloader->isFinished()) {
        if (bytesTotal == 0)
            return;

        //qDebug() << QString("onDownloadProgress(%1 / %2)").arg(bytesReceived).arg(bytesTotal);
        qint64 curTimeMs = QDateTime::currentMSecsSinceEpoch();
        if (curTimeMs - m_startTimeMs == 0) {
            return;
        }

        if (m_timer) {
            m_timer->start();
        }
        static qint64 lastTimeMs = QDateTime::currentMSecsSinceEpoch();
        double speed = static_cast<double>(bytesReceived) / (curTimeMs - m_startTimeMs);
        int remainTime = static_cast<int>((bytesTotal - bytesReceived) / speed / 1000);
        if (curTimeMs - lastTimeMs > 500) {
            lastTimeMs = curTimeMs;
            //qDebug() << QString("remainTime(%1 s)").arg(remainTime);
            int min    = remainTime / 60;
            int second = remainTime % 60;
            QString time;
            if (min == 0) {
                time = QString(tr("%1 seconds")).arg(second);
            } else {
                time = QString(tr("%1 minutes")).arg(min);
            }

            double mb = static_cast<double>(bytesReceived) / 1024 / 1024;
            // 已下载%1/%2，还剩%3分钟
            m_pLabelSummary->setText(QString(tr("%1MB/%2GB downloaded, %3 left.")).arg(QString::number(mb, 'f', 3)).arg(m_sizeGb).arg(time));
            adjustSummaryLabelWidth();
            if (!m_pSpinner->isVisible()) {
                m_pSpinner->setVisible(true);
                m_pSpinner->start();
            }
        }
    }
}

ModelScopeItem::ItemInfo ModelScopeItem::ItemInfo::deepseek_r1_1_5B()
{
    ItemInfo info;
    info.appName = "DeepSeek-R1-Distill-Qwen-1.5B-Q4_K_M";
    info.modelName = "DeepSeek-R1-1.5B";
    info.baseUrl = "https://www.modelscope.cn/models/uniontech-yourong";
    info.modelFileList  << "modelhub/config.json"
                        << "modelhub/LICENSE"
                        << "modelhub/template"
                        << "DeepSeek-R1-Distill-Qwen-1.5B-Q4_K_M.gguf"
                        << "sha256";
    return info;
}

ModelScopeItem::ItemInfo ModelScopeItem::ItemInfo::yourongv1_1_5B()
{
    ItemInfo info;
    info.appName = "yourongv1-1.5B-Instruct-GGUF";
    info.modelName = "YouRong-1.5B";
    info.baseUrl = "https://www.modelscope.cn/models/uniontech-yourong";
    info.modelFileList  << "modelhub/config.json"
                        << "modelhub/LICENSE"
                        << "modelhub/template"
                        << "modelhub/template_dsl"
                        << "modelhub/template_func"
                        << "sha256"
                        << "yourong_q4_km.gguf";
    return info;
}

ModelScopeItem::ItemInfo ModelScopeItem::ItemInfo::yourongv1_7B()
{
    ItemInfo info;
    info.appName = "yourongv1-7B-Instruct-GGUF";
    info.modelName = "YouRong-7B";
    info.baseUrl = "https://www.modelscope.cn/models/uniontech-yourong";
    info.modelFileList  << "modelhub/config.json"
                        << "modelhub/LICENSE"
                        << "modelhub/template"
                        << "modelhub/template_dsl"
                        << "modelhub/template_func"
                        << "sha256"
                        << "yourongv1-7B-Instruct-q4km.gguf";
    return info;
}
