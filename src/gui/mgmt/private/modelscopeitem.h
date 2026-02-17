// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MODELSCOPEITEM_H
#define MODELSCOPEITEM_H

#include "uosai_global.h"
#include "updatebutton.h"
#include <QProcess>
#include <QTimer>

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DSuggestButton>
#include <DSpinner>

namespace uos_ai {

class Downloader;
class ModelUpdater;

class ModelScopeItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    struct ItemInfo
    {
        ItemInfo() {}
        QString appName;
        QString modelName;
        QString baseUrl;
        QStringList modelFileList;

        static ItemInfo deepseek_r1_1_5B();
        static ItemInfo yourongv1_1_5B();
        static ItemInfo yourongv1_7B();
    };

    explicit ModelScopeItem(const ItemInfo &info, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    virtual ~ModelScopeItem();

    void stopDownload();
    void setText(const QString &theme, const QString &summary, double sizeGb);
    void checkInstallStatus();
    void saveNewHashFile();

Q_SIGNALS:
    void sigRedPointVisible(bool);

protected:
    virtual void initUI();
    virtual void initConnect();
    virtual void changeInstallStatus();
    virtual void adjustSummaryLabelWidth();

    virtual void onInstall();
    virtual void onUpdate();
    virtual void onUninstall();
    virtual void startDownload();

    bool getInstallStatus();
    void checkUpdateStatus();
    static void runCheckFile(ModelScopeItem *self, const QString &dirPath, const QStringList &fileList);

protected slots:
    virtual void onCheckFile();
    virtual void onCheckFileFailed();
    virtual void onDownloadFinished();
    virtual void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
protected:
    void resizeEvent(QResizeEvent *event) override;
protected:
    ItemInfo m_info;
    bool m_isInstall = false;
    double m_lastProgress = 0.0;
    QString m_installPath;
    QSharedPointer<Downloader> m_downloader;
    double m_sizeGb = 0.0;
    qint64 m_startTimeMs = 0;
    QTimer *m_timer = nullptr;
    QScopedPointer<ModelUpdater> m_updater;

    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelTheme = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelSize = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelSummary = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pBtnInstall = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_pBtnUninstall = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_pBtnStop = nullptr;
    DTK_WIDGET_NAMESPACE::DSpinner *m_pSpinner = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_redPoint = nullptr;
    UpdateButton *m_pBtnUpdate = nullptr;
};

}

#endif // MODELSCOPEITEM_H
