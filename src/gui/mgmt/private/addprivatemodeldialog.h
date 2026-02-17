#ifndef ADDPRIVATEMODELDIALOG_H
#define ADDPRIVATEMODELDIALOG_H
#include "uosai_global.h"
#include "serverdefs.h"

#include <DAbstractDialog>
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

namespace uos_ai {

class AddPrivateModelDialog: public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit AddPrivateModelDialog(QWidget *parent = nullptr);

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
    void onUpdateSubmitButtonStatus();
    void onNameTextChanged(const QString &);
    void onNameAlertChanged(bool alert);
    void onCompactModeChanged();
    void onUpdateSystemFont(const QFont &);

    void updateProxyLabel();
private:
    DTK_WIDGET_NAMESPACE::DPasswordEdit *m_pApiKeyLineEdit = nullptr;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_pNameLineEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DLineEdit *m_pCustomModelName = nullptr;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_pCustomModelUrl = nullptr;

    DTK_WIDGET_NAMESPACE::DPushButton *m_pCancelButton = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_pSubmitButton = nullptr;
    QGridLayout *m_pGridLayout = nullptr;
    DTK_WIDGET_NAMESPACE::DSpinner *m_pSpinner = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pProxyLabel = nullptr;
    QWidget *m_pWidget = nullptr;
    LLMServerProxy m_data;

    QSet<QWidget *> m_widgetSet;
};
}

#endif // ADDPRIVATEMODELDIALOG_H
