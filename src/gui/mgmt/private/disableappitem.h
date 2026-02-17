#ifndef DISABLEAPPITEM_H
#define DISABLEAPPITEM_H
#include "uosai_global.h"

#include <DFrame>
#include <DLabel>
#include <DIconButton>
#include <QPaintEvent>
#include <QEvent>

UOSAI_BEGIN_NAMESPACE

class DisableAppItem : public DTK_WIDGET_NAMESPACE::DFrame
{
    Q_OBJECT

public:
    explicit DisableAppItem(const QString &displayName, const QString &iconName, const QString &originalAppName, QWidget *parent = nullptr);
    ~DisableAppItem();

    void setAppName(const QString &name);
    void setAppIcon(const QString &iconName);
    QString appName() const;

signals:
    void deleteClicked(const QString &appName);

protected:
    void paintEvent(QPaintEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    void initUI();
    void onThemeTypeChanged();
    void updateIconPixmap();

    QString m_appName;
    QString m_originalAppName;
    QString m_iconName;
    bool m_isHovered;

    DTK_WIDGET_NAMESPACE::DLabel *m_iconLabel;
    DTK_WIDGET_NAMESPACE::DLabel *m_nameLabel;
    DTK_WIDGET_NAMESPACE::DIconButton *m_deleteBtn;
};

UOSAI_END_NAMESPACE
#endif // DISABLEAPPITEM_H
