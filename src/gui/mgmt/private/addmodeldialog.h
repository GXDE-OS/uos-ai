#ifndef ADDMODELDIALOG_H
#define ADDMODELDIALOG_H

#include "serverdefs.h"

#include <DAbstractDialog>
#include <DWidget>
#include <DLabel>
#include <DComboBox>
#include <DLineEdit>
#include <DPushButton>
#include <DTitlebar>
#include <DFontSizeManager>
#include <DPasswordEdit>
#include <DSuggestButton>
#include <DSpinner>

#include <QGridLayout>
#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE

class RaidersButton;

class AddModelDialog: public DAbstractDialog
{
    Q_OBJECT

public:
    explicit AddModelDialog(DWidget *parent = nullptr);

    LLMServerProxy getModelData();

    void resetDialog();

private:
    void initUI();
    void initConnect();

    void setAllWidgetEnabled(bool);
    /**
     * @brief 账号名称是否重复，重复时m_pNameLineEdit发出提示
     * @return true重复;false不重复
     */
    bool isNameDuplicate(const QList<LLMServerProxy> &) const;

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSubmitButtonClicked();
    void onComboBoxIndexChanged(int index);
    void onUpdateSubmitButtonStatus();
    void onNameTextChanged(const QString &);
    void onNameAlertChanged(bool alert);
    void onAddressTextChanged(const QString &);
    void onAddressAlertChanged(bool alert);
    void onCompactModeChanged();
    void onUpdateSystemFont(const QFont &);

    void updateProxyLabel();
private:
    DComboBox *m_pModelComboBox = nullptr;
    DPasswordEdit *m_pAppIdLineEdit = nullptr;
    DPasswordEdit *m_pApiKeyLineEdit = nullptr;
    DPasswordEdit *m_pApiSecretLineEdit = nullptr;
    DLineEdit *m_pNameLineEdit = nullptr;
    DLineEdit *m_pAddressLineEdit = nullptr;

    DLineEdit *m_pCustomModelName = nullptr;
    DLineEdit *m_pCustomModelUrl = nullptr;

    DPushButton *m_pCancelButton = nullptr;
    DSuggestButton *m_pSubmitButton = nullptr;
    QGridLayout *m_pGridLayout = nullptr;
    DSpinner *m_pSpinner = nullptr;
    DLabel *m_pProxyLabel = nullptr;
    QWidget *m_pWidget = nullptr;
    RaidersButton *m_pRaidersButton = nullptr;
    LLMServerProxy m_data;

    QSet<QWidget *> m_widgetSet;
    QMap<int, LLMChatModel> m_modelMap;
    QSet<LLMChatModel> m_threeKeyComboxIndex; //拥有3个字段的combox索引
    int m_lastModelIndex = -1;
};

#endif // ADDMODELDIALOG_H
