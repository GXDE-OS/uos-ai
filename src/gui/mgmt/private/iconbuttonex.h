#ifndef ICONBUTTONEX_H
#define ICONBUTTONEX_H

#include "mgmtdefs.h"

#include <DWidget>
#include <DFontSizeManager>
#include <DLabel>
#include <DSpinner>
#include <DGuiApplicationHelper>
#include <DTipLabel>
#include <QScreen>

namespace uos_ai {
class IconButtonEx : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit IconButtonEx(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    explicit IconButtonEx(const QString text, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void setText(const QString &);
    void setFont(DTK_WIDGET_NAMESPACE::DFontSizeManager::SizeType type, int weight);
    void setIconSize(const QSize &);
    void setHighlight(bool);
    void setSpacing(int);
    void setInterruptFilter(bool);

    void setStatusIcon(const QString &iconName);
    void setTipsIcon(const QString &iconName);

    void setSpinnerVisible(bool visible);

    void showTips(int x, int y);
    void hideTips();

    void setStatus(KnowledgeBaseProcessStatus status);

    DTK_WIDGET_NAMESPACE::DLabel *getTipsIcon();

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onThemeTypeChanged(DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType themeType);
    void onApplicationPaletteChanged();

private:
    DTK_WIDGET_NAMESPACE::DLabel *m_pStatusIcon = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pIcon = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pTipsIcon = nullptr;
    DTK_WIDGET_NAMESPACE::DSpinner *m_spinner = nullptr;

    DTK_WIDGET_NAMESPACE::DTipLabel *m_tips = nullptr;

    bool m_bHighlight = false;
    bool m_bInterrupt = false;
    KnowledgeBaseProcessStatus m_knowledgeBaseProcessStatus = KnowledgeBaseProcessStatus::Succeed;
    QSize m_iconSize;
};

}
#endif // ICONBUTTONEX_H
