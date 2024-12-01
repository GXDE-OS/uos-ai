#ifndef ICONBUTTONEX_H
#define ICONBUTTONEX_H

#include "mgmtdefs.h"

#include <DWidget>
#include <DFontSizeManager>
#include <DLabel>
#include <DSpinner>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class IconButtonEx : public DWidget
{
    Q_OBJECT

public:
    explicit IconButtonEx(DWidget *parent = nullptr);
    explicit IconButtonEx(const QString text, DWidget *parent = nullptr);

    void setText(const QString &);
    void setFont(DFontSizeManager::SizeType type, int weight);
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

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);
    void onApplicationPaletteChanged();

private:
    DLabel *m_pStatusIcon = nullptr;
    DLabel *m_pLabel = nullptr;
    DLabel *m_pIcon = nullptr;
    DLabel *m_pTipsIcon = nullptr;
    DSpinner *m_spinner = nullptr;

    DLabel *m_tips = nullptr;

    bool m_bHighlight = false;
    bool m_bInterrupt = false;
    KnowledgeBaseProcessStatus m_knowledgeBaseProcessStatus = KnowledgeBaseProcessStatus::Succeed;
    QSize m_iconSize;
};

#endif // ICONBUTTONEX_H
