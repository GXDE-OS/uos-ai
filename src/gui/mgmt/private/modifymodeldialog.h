#ifndef MODIFYMODELDIALOG_H
#define MODIFYMODELDIALOG_H

#include "serverdefs.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

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
#include <DPushButton>

DWIDGET_USE_NAMESPACE

class ModifyModelDialog: public DAbstractDialog
{
    Q_OBJECT

public:
    explicit ModifyModelDialog(const LLMServerProxy &data, DWidget *parent = nullptr);
    ~ModifyModelDialog() = default;

    QString getModelName();

private:
    void setData(const LLMServerProxy &data);

    void updateContexts(const LLMChatModel &model);

    QString getDesensitivity(const QString &input);

    bool isNameDuplicate(const QList<LLMServerProxy> &llmList) const;

private slots:
    void onNameTextChanged(const QString &);
    void onNameAlertChanged(bool alert);
    void onUpdateSystemFont(const QFont &);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    DLabel *m_pModelLabel = nullptr;
    DLabel *m_pAppIdLabel = nullptr;
    DLabel *m_pApiKeyLabel = nullptr;
    DLabel *m_pApiSecretLabel = nullptr;

    DLineEdit *m_pNameLineEdit = nullptr;

    DPushButton *m_pSubmitButton = nullptr;

    QGridLayout *m_pContextLayout = nullptr;

    QString m_appId;
    QString m_apiKey;
    QString m_apiSecret;
    QString m_name;

    QSet<LLMChatModel> m_threeKeyComboxIndex;
};

#endif // MODIFYMODELDIALOG_H
