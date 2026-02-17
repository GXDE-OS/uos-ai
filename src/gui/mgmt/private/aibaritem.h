#ifndef AIBARITEM_H
#define AIBARITEM_H

#include "uosai_global.h"
#include <DWidget>
#include <DLabel>
#include <DSwitchButton>

DWIDGET_USE_NAMESPACE
namespace uos_ai {
class AiBarItem: public DWidget
{
    Q_OBJECT
public:
    explicit AiBarItem(DWidget *parent = nullptr);
    void setSwitchChecked(bool);
    ~AiBarItem();

private:
    void initUI();
    void initConnect();

public:
    void setText(const QString &theme, const QString &summary);

signals:
    void signalSwitchChanged(bool);

private slots:

private:
    DLabel *m_pLabelTheme = nullptr;
    DLabel *m_pLabelSummary = nullptr;
    DSwitchButton *m_pBtnSwitch = nullptr;
};
}
#endif // AIBARITEM_H
