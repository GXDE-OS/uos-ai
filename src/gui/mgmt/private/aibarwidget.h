#ifndef AIBARWIDGET_H
#define AIBARWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>
#include "uosai_global.h"

DWIDGET_USE_NAMESPACE

class ThemedLable;
namespace uos_ai {
class AiBarItem;
class AiBarWidget: public DWidget
{
    Q_OBJECT

public:
    explicit AiBarWidget(DWidget *parent = nullptr);
    void updateDragStatus(bool enable);
    QString getTitleName();

signals:
    void signalChangeDragStatus(bool isHidden);

private slots:
    void onThemeTypeChanged();

private:
    void initUI();
    DBackgroundGroup *aiBarWidget();

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DBackgroundGroup *m_pAiBarWidget = nullptr;
    AiBarItem *m_AiBarItem = nullptr;
};
}
#endif // AIBARWIDGET_H
