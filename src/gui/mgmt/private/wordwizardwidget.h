#ifndef WORDWIZARDWIDGET_H
#define WORDWIZARDWIDGET_H
#include "uosai_global.h"
#include "disableappwidget.h"
#include "skilllistwidget.h"

#include <QLayout>

#include <DWidget>
#include <DBackgroundGroup>

class ThemedLable;

namespace uos_ai {
class WordWizardItem;
class DisableAppWidget;
class WizardWrapperLabel;
class WordWizardWidget: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit WordWizardWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    void updateHiddenStatus(bool isHidden);
    QString getTitleName();
    DisableAppWidget* getDisableAppWidget() { return m_pDisableWidget; }

signals:
    void signalChangeHiddenStatus(bool isHidden);
    void disabledAppAdded(const QString &appName);
    void disabledAppsUpdateRequested(const QStringList &appList);
    void skillAddedSuccessfully(const QString &message);

private slots:
    void onThemeTypeChanged();
    void onDisableWidgetEmpty();
    void onDisableAppAdded(const QString &appName);

private:
    void initUI();
    void initConnect();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *wordWizardWidget();
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *disableAppWidget();

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pWordWizardWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_pDisableAppWidget = nullptr;
    WordWizardItem *m_WordWizardItem = nullptr;
    DisableAppWidget *m_pDisableWidget = nullptr;
    WizardWrapperLabel *m_imageLabel = nullptr;
    SkillListWidget *m_pSkillListWidget = nullptr;
};
}

#endif // WORDWIZARDWIDGET_H
