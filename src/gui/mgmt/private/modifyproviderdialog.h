#ifndef MODIFYPROVIDERDIALOG_H
#define MODIFYPROVIDERDIALOG_H

#include "model/modelinfo.h"
#include "raidersbutton.h"

#include <QGridLayout>

#include <DAbstractDialog>
#include <DLabel>
#include <DLineEdit>
#include <DTitlebar>
#include <DCheckBox>
#include <DSuggestButton>
#include <DComboBox>
#include <DPasswordEdit>
#include <DListWidget>
#include <DBackgroundGroup>
#include <DCommandLinkButton>
#include <DIconButton>
#include <DSpinner>

namespace uos_ai {
class BuiltinModelItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit BuiltinModelItem(QWidget *parent = nullptr);
    inline void setId(const QString &id) { m_id = id; }
    inline QString id() const { return m_id; }
    inline void setName(const QString &name) { m_checkBox->setText(name); }

    void setEnableCheck(bool enable);
    void setChecked(bool);
    bool isChecked() const;
    inline void setHideTest(bool hide) {m_hideTest = hide;}
protected:
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
signals:
    void requestCheck();
private:
    QString m_id;
    bool m_hideTest = false;
    DTK_WIDGET_NAMESPACE::DCheckBox *m_checkBox = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_testButton = nullptr;
};

class CustomModelGroup : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit CustomModelGroup(QWidget *parent = nullptr);
    void setEditMode(bool edit);
    inline void setId(const QString &id) { m_id = id; }
    inline QString id() const { return m_id; }

    inline void setName(const QString &name) { m_nameEdit->setText(name); }
    inline QString name() const { return m_nameEdit->text().trimmed(); }

    inline void setModelId(const QString &id) { m_idEdit->setText(id); }
    inline QString modelId() const { return m_idEdit->text().trimmed(); }

    void setReadOnly(bool b);
    void setEnableTest(bool enable);
signals:
    void requestRemove();
    void requestCheck();
    void modelChanged();
private:
    QString m_id;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_nameEdit = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_testButton = nullptr;
    DTK_WIDGET_NAMESPACE::DIconButton *m_deleteButton = nullptr;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_idEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_group = nullptr;
};

class ModifyProviderDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit ModifyProviderDialog(bool priavte = false, QWidget *parent = nullptr);
    explicit ModifyProviderDialog(const ProviderAccount &provider, const QList<ModelAccountPtr> &models,
                                  bool priavte = false, QWidget *parent = nullptr);
    void updateProxyLabel();
    void resetEdit();
signals:
    void dataChanged();
private:
    void initUI();
    void initConnect();
    void initEmpty();
    void initData();

    bool isPrivateOrOpenAICompatible(const QString &id) const;
    QString getDesensitivity(const QString &input);

    void clearBuiltinModel();

    bool confirmEdit();
    bool confirmAdd();

    CustomModelGroup *createCustomModel(bool pre = true);
    BuiltinModelItem *createBuiltinModel(bool pre = true);

    void setEnableInput(bool);
private slots:
    void onCancelButtonClicked();
    void onConfirmButtonClicked();
    void onEdit();

    void onDeleteModel();
    void onAddModelClicked();
    void onTestModelClicked();
    void onProviderChanged(int index);

    void switchButtonStatus();
    void onFontChanged();

private:
    bool m_editMode = false;
    bool m_onlyPrivate = false;
    ProviderAccount m_provider;
    QList<ModelAccountPtr> m_models;
    QStringList m_providerList;

    QGridLayout *m_gridLayout = nullptr;
    DTK_WIDGET_NAMESPACE::DTitlebar *m_titleBar = nullptr;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_accountEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DComboBox *m_providerComboBox = nullptr;
    DTK_WIDGET_NAMESPACE::DPasswordEdit *m_apiKeyEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DLineEdit *m_domainEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DWidget *m_bulitinContent = nullptr;

    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_editButton = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_addButton = nullptr;

    DTK_WIDGET_NAMESPACE::DWidget *m_customContent = nullptr;

    DTK_WIDGET_NAMESPACE::DLabel *m_warningLabel = nullptr;
#if 0
    RaidersButton *m_raidersButton = nullptr;
#endif
    DTK_WIDGET_NAMESPACE::DLabel *m_proxyLabel = nullptr;

    DTK_WIDGET_NAMESPACE::DSpinner *m_pSpinner = nullptr;

    DTK_WIDGET_NAMESPACE::DPushButton *m_cancelButton = nullptr;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_confirmButton = nullptr;
};

}

#endif // MODIFYPROVIDERDIALOG_H
