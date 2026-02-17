#ifndef UPDATELOGDIALOG_H
#define UPDATELOGDIALOG_H

#include "uosai_global.h"

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include <DScrollArea>
#include <DGuiApplicationHelper>
#include <DIconButton>

#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

struct UpdateLogItem {
    QString type;        // "new", "improvement", "fix"
    QString description;
};

struct UpdateLogVersion {
    QString version;
    QString date;
    QList<UpdateLogItem> items;
};

class UpdateLogDialog : public DDialog
{
    Q_OBJECT
public:
    explicit UpdateLogDialog(QWidget *parent = nullptr);

private slots:
    void onUpdateSystemTheme(const DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType &);

private:
    void initUI();
    void initConnect();
    void loadUpdateLogs();
    QHBoxLayout* createVersionWidget(const UpdateLogVersion &version);
    QHBoxLayout* createItemWidget(const UpdateLogItem &item, int index);
    
private:
    DScrollArea *m_scrollArea;
    DWidget *m_contentWidget;
    QVBoxLayout *m_contentLayout;
    
    QList<UpdateLogVersion> m_updateLogs;
};

} // namespace uos_ai

#endif // UPDATELOGDIALOG_H
