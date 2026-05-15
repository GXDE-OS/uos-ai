#ifndef MGMTWINDOW_H
#define MGMTWINDOW_H

#include <DMainWindow>
#include <DWidget>
#include <DStandardItem>
#include <DListView>

#include <QScrollArea>

namespace uos_ai {

class PrivateModelListWidget;
class AiBarWidget;
class WordWizardWidget;
class GetFreeAccountDialog;
class KnowledgeBaseListWidget;
class ModelListWidget;
class LocalModelListWidget;
class Navigation;
class McpServerWidget;
class ChatBotWidget;
class MgmtWindow : public DTK_WIDGET_NAMESPACE::DMainWindow
{
    Q_OBJECT

public:
    enum Page {
        Default = 0,
        ModelList,
        KnowledgeBase,
        FollowAlong,
        AddModel
    };

    explicit MgmtWindow(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~MgmtWindow();

    void showPage(int page);
    void checkUpdateStatus();

public slots:
    void showGetFreeAccountDlg();
private slots:
    void onThemeTypeChanged();
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
    void loadDisabledApps();

    ModelListWidget *initModelListWidget();
    PrivateModelListWidget *initPrivateModelListWidget();
    LocalModelListWidget *initLocalModelListWidget();
    DTK_WIDGET_NAMESPACE::DWidget *initAgreementWidget();
    DTK_WIDGET_NAMESPACE::DWidget *initProxyWidget();
    KnowledgeBaseListWidget *initKnowledgeBaseWidget();
    WordWizardWidget *initWordWizardWidget();
    DTK_WIDGET_NAMESPACE::DWidget *initMcpServerWidget();
    AiBarWidget *initAiBarWidget();
    DTK_WIDGET_NAMESPACE::DWidget *initModelConfigWidget();
    ChatBotWidget *initChatBotWidget();

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
    QMap<QString, DTK_WIDGET_NAMESPACE::DWidget *> titles = {};
    QList<DTK_WIDGET_NAMESPACE::DWidget *> widgetList = {};
    QWidget *m_pToastContent = nullptr;
    QScrollArea *m_pScrollArea = nullptr;

    Navigation *m_pNavigationWidget = nullptr;
    ModelListWidget *m_pModelListWidget = nullptr;
    LocalModelListWidget *m_pLocalModelListWidget = nullptr;
    KnowledgeBaseListWidget *m_pKnowledgeBaseListWidget = nullptr;
    WordWizardWidget *m_pWordWizardWidget = nullptr;
    AiBarWidget *m_pAiBarWidget = nullptr;
    McpServerWidget *m_pMcpServerWidget = nullptr;
    PrivateModelListWidget *m_pPrivateModelListWidget = nullptr;
    ChatBotWidget *m_pChatBotWidget = nullptr;

    QSet<QWidget *> m_widgets;

    bool m_bIsWordWizardHidden = false;
    GetFreeAccountDialog *m_pGetFreeAccountDialog = nullptr;
};

}

#endif // MGMTWINDOW_H
