#ifndef WORDWIZARDITEM_H
#define WORDWIZARDITEM_H
#include "uosai_global.h"

#include <DWidget>
#include <DLabel>
#include <DSwitchButton>

namespace uos_ai {
class WordWizardItem: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit WordWizardItem(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    void setSwitchChecked(bool);
    ~WordWizardItem();

private:
    void initUI();
    void initConnect();

public:
    void setText(const QString &theme, const QString &summary);

signals:
    void signalSwitchChanged(bool);

private slots:

private:
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelTheme = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabelSummary = nullptr;
    DTK_WIDGET_NAMESPACE::DSwitchButton *m_pBtnSwitch = nullptr;
};
}

#endif // WORDWIZARDITEM_H
