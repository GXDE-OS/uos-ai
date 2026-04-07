#ifndef MGMTWINDOW_H
#define MGMTWINDOW_H
#include "private/navigation.h"
#include "private/disableappwidget.h"

#include <QScrollArea>

#include <DMainWindow>
#include <DWidget>
#include <DStandardItem>
#include <DListView>

DWIDGET_USE_NAMESPACE

namespace uos_ai {
class PrivateModelListWidget;
class AddPrivateModelDialog;
class AiBarWidget;
class WordWizardWidget;
class McpServerWidget;
class SkillServerWidget;
class ChatBotWidget;
};

class ModelListWidget;
class LocalModelListWidget;
class AddModelDialog;
class KnowledgeBaseListWidget;
class GetFreeAccountDialog;

class MgmtWindow : public DMainWindow
{
    Q_OBJECT

public:
    explicit MgmtWindow(DWidget *parent = nullptr);
    ~MgmtWindow();

    void showEx(bool, bool onlyUseAgreement = false, bool isFromAiQuick = false, const QString & locateTitle = "");
    void checkUpdateStatus();
    void showGetFreeAccountDlg();

private slots:
    void onAddModel();
    void showAddPrivateModel();
    void onThemeTypeChanged();
    void onShowGetFreeAccountDialog();
    void onscrollAreaValueChanged(int value);
    void onNavigationSelected(const QString & key);

public slots:
    void onWordWizardHiddenStatus(bool isHidden);
    void onHiddenGetFreeAccountBtn();
    void onAddKnowledgeBase(const QStringList & filePath);
    void onAddDisabledApp(const QString &appName);
    void showFloatingMessage(const QString &message);

private:
    void initUI();
    void initConnect();
    void onAddPrivateModel();
    void loadDisabledApps();

    ModelListWidget *initModelListWidget();
    uos_ai::PrivateModelListWidget *initPrivateModelListWidget();
    LocalModelListWidget *initLocalModelListWidget();
    DWidget *initAgreementWidget();
    DWidget *initProxyWidget();
    KnowledgeBaseListWidget *initKnowledgeBaseWidget();
    uos_ai::WordWizardWidget *initWordWizardWidget();
    uos_ai::AiBarWidget *initAiBarWidget();
    DWidget *initMcpServerWidget();
    DWidget *initSkillWidget();
    DWidget *initModelConfigWidget();
    DWidget *initChatBotWidget();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void signalCloseWindow();
    void sigGenPersonalFAQ();
    void scrollToGroup(const QString &key);
    void signalWordWizardStatusChanged(bool isHidden);
    void sigSetRedPointVisible(bool);
    void signalDisabledAppsUpdated(const QStringList &appList);
    void sigThirdPartyMcpAgree();

private:
    QMap<QString, DWidget *> titles = {};
    QList<DWidget *> widgetList = {};
    QWidget *m_pToastContent = nullptr;
    QScrollArea *m_pScrollArea = nullptr;

    uos_ai::Navigation *m_pNavigationWidget = nullptr;
    ModelListWidget *m_pModelListWidget = nullptr;
    LocalModelListWidget *m_pLocalModelListWidget = nullptr;
    KnowledgeBaseListWidget *m_pKnowledgeBaseListWidget = nullptr;
    uos_ai::WordWizardWidget *m_pWordWizardWidget = nullptr;
    uos_ai::AiBarWidget *m_pAiBarWidget = nullptr;
    uos_ai::McpServerWidget *m_pMcpServerWidget = nullptr;
    uos_ai::SkillServerWidget *m_pSkillServerWidget = nullptr;
    uos_ai::PrivateModelListWidget *m_pPrivateModelListWidget = nullptr;
    uos_ai::ChatBotWidget *m_pChatBotWidget = nullptr;


    QSet<QWidget *> m_widgets;
    AddModelDialog *m_pAddDlg = nullptr;
    uos_ai::AddPrivateModelDialog *m_pAddPrivateDlg = nullptr;

    bool m_bIsWordWizardHidden = false;
    GetFreeAccountDialog *m_pGetFreeAccountDialog=nullptr;

    bool m_isFromAiQuick = false;
};


#endif // MGMTWINDOW_H
