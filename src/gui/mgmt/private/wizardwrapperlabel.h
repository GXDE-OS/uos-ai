#ifndef WIZARDWRAPPERLABEL_H
#define WIZARDWRAPPERLABEL_H

#include <DLabel>
#include <DGuiApplicationHelper>

#include <QList>
#include <QIcon>
#include <QPainterPath>

namespace uos_ai {
class WizardWrapperLabel : public DTK_WIDGET_NAMESPACE::DLabel
{
    Q_OBJECT
public:
    explicit WizardWrapperLabel(QWidget *parent = nullptr);
    ~WizardWrapperLabel() override;
    void initFunctionButtons();
    void calculateWidth();
    void setRadius(int xRadius, int yRadius);

    QWidget* createWrapper(QWidget* parent = nullptr);

public slots:
    void onSkillListChanged();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onUpdateSystemFont(const QFont &);
    void onThemeTypeChanged();

private:
    void drawWizardWrapper(QPainter &painter);
    void drawButtons(QPainter &painter, int startX, int y);
    QColor getMaskColor() const;
    
    DTK_WIDGET_NAMESPACE::DLabel *m_helloLabel = nullptr;
    int m_xRadius = 8;
    int m_yRadius = 8;
    double m_maskAlpha = 0.6;
    QColor m_backgroundColor;
    QColor m_textColor;

    int m_arrowWidth = 16;
    int m_arrowHeight = 12;
    int m_arrowX = -1;
    bool m_radiusArrowStyleEnable = true; // 是否启用圆角箭头样式
    
    QIcon m_iconBtnIcon;
    QIcon m_moreBtnIcon;
    QIcon m_closeBtnIcon;
};
}

#endif // WIZARDWRAPPERLABEL_H 
