#include "themedbutton.h"

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOptionButton>

using namespace uos_ai;
DWIDGET_USE_NAMESPACE

ThemedButton::ThemedButton(QWidget *parent)
    : DPushButton(parent)
{
    init();
}

ThemedButton::ThemedButton(const QString &text, QWidget *parent)
    : DPushButton(text, parent)
{
    init();
}

ThemedButton::~ThemedButton()
{
}

void ThemedButton::setButtonStyle(ButtonStyle style)
{
    if (m_buttonStyle != style) {
        m_buttonStyle = style;
        updateStyle();
    }
}

ThemedButton::ButtonStyle ThemedButton::buttonStyle() const
{
    return m_buttonStyle;
}

void ThemedButton::init()
{
    updateStyle();
    connect(DTK_GUI_NAMESPACE::DGuiApplicationHelper::instance(), &DTK_GUI_NAMESPACE::DGuiApplicationHelper::themeTypeChanged, this, &ThemedButton::updateStyle);
}

void ThemedButton::updateStyle()
{
    bool isDarkTheme = DTK_GUI_NAMESPACE::DGuiApplicationHelper::instance()->themeType() == DTK_GUI_NAMESPACE::DGuiApplicationHelper::DarkType;
    
    QString bgColor = isDarkTheme ? "rgba(0, 0, 0, 0.2)" : "rgba(0, 0, 0, 0.15)";
    QString hoverBgColor = isDarkTheme ? "rgba(255, 255, 255, 0.1)" : "rgba(0, 0, 0, 0.1)";
    QString pressedBgColor = isDarkTheme ? "rgba(0, 0, 0, 0.4)" : "rgba(0, 0, 0, 0.3)";
    
    QString defaultBorder = isDarkTheme ? "border-top: 1px solid rgba(255, 255, 255, 0.05); border-bottom: 1px solid rgba(0, 0, 0, 0.4); border-left: none; border-right: none;" : "border: none;";
    QString hoverBorder = isDarkTheme ? "border-top: 1px solid rgba(255, 255, 255, 0.1); border-bottom: 1px solid rgba(0, 0, 0, 0.5); border-left: none; border-right: none;" : "border: 1px solid rgba(0, 0, 0, 0.1);";
    
    if (m_buttonStyle == Default) {
        QString enabledColor = isDarkTheme ? "rgba(40, 132, 255, 1)" : "rgba(0, 88, 222, 1)";
        QString disabledColor = isDarkTheme ? "rgba(40, 132, 255, 0.4)" : "rgba(0, 88, 222, 0.4)";
        QString pressedColor = "rgba(0, 88, 222, 1)";
        
        setStyleSheet(QString("QPushButton { background-color: %1; border-radius: 6px; %4 }"
                              "QPushButton:hover { background-color: %5; %6 }"
                              "QPushButton:pressed { background-color: %7; color: %8; }"
                              "QPushButton:enabled { color: %2; } QPushButton:disabled { color: %3; }")
                      .arg(bgColor).arg(enabledColor).arg(disabledColor).arg(defaultBorder).arg(hoverBgColor).arg(hoverBorder).arg(pressedBgColor).arg(pressedColor));
    } else {
        // Plain style - 不改变文字颜色
        setStyleSheet(QString("QPushButton { background-color: %1; border-radius: 6px; %2 }"
                              "QPushButton:hover { background-color: %3; %4 }"
                              "QPushButton:pressed { background-color: %5; }")
                      .arg(bgColor).arg(defaultBorder).arg(hoverBgColor).arg(hoverBorder).arg(pressedBgColor));
    }
}

void ThemedButton::paintEvent(QPaintEvent *event)
{
    DPushButton::paintEvent(event);
}
