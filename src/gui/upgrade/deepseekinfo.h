// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEEPSEEKINFO_H
#define DEEPSEEKINFO_H

#include "uosai_global.h"
#include "tasdef.h"
#include "uosfreeaccounts.h"

#include <DDialog>
#include <DWidget>
#include <DSuggestButton>
#include <DSpinner>
#include <DCommandLinkButton>

#include <QFutureWatcher>
#include <QAbstractButton>
#include <QSpacerItem>

class ThemedLable;
namespace uos_ai {

enum ReceivedStatus {
    CHECKING = 0,
    ACTIVITY_EXIST,
    ACTIVITY_OVER,
    CHECK_FAIL,
    RECEIVING,
    RECEIVE_SUCCEED,
    RECEIVE_FAIL
};

class DeepSeekInfo : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT
public:
    static DeepSeekInfo &getInstance();
    void checkActivity();
    static bool checkAndShow();
    static bool needGuide();

private:
    explicit DeepSeekInfo(QWidget *parent = nullptr);
    void initUI();
    void initConnect();
    void onGetAccount();
    void adjustReceiveBtn(int type);
    void hideAllComponents();
    void changeIntroduceText(int type);

protected slots:
    void getAccount();
    void onCheckActivity();
    void onUpdateSystemFont(const QFont &);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QFutureWatcher<int> watcher;

    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pFreeAccount = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_pFreeWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_pLoadingWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DSpinner *m_pSpinner = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_pLaterLabel = nullptr;
    ThemedLable *m_pFirstIntroduce = nullptr;
    ThemedLable *m_pSecondIntroduce = nullptr;
    ThemedLable *m_pLoadingLabel = nullptr;
    ThemedLable *m_pExplainLabel = nullptr;
    QSpacerItem *m_pSpacer = nullptr;

    LLMServerProxy enrie;
    static bool isDialogOpen;
};
}

#endif // DEEPSEEKINFO_H
