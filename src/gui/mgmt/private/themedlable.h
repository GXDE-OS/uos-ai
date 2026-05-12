#ifndef THEMEDLABLE_H
#define THEMEDLABLE_H

#include <DLabel>
#include <DPalette>

#include <QWidget>

namespace uos_ai {

class ThemedLable : public DTK_WIDGET_NAMESPACE::DLabel
{
public:
    explicit ThemedLable(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ThemedLable(const QString &text, QWidget *parent = nullptr);

    void setPaletteColor(DTK_GUI_NAMESPACE::DPalette::ColorRole, DTK_GUI_NAMESPACE::DPalette::ColorType, qreal alphaF = 1.0);
    void setPaletteColor(DTK_GUI_NAMESPACE::DPalette::ColorRole, DTK_GUI_NAMESPACE::DPalette::ColorRole, qreal alphaF = 1.0);

private slots:
    void onThemeTypeChanged();

private:
    QMap<DTK_GUI_NAMESPACE::DPalette::ColorRole,  std::pair<DTK_GUI_NAMESPACE::DPalette::ColorType, qreal>> m_colorTypeMap;
    QMap<DTK_GUI_NAMESPACE::DPalette::ColorRole,  std::pair<DTK_GUI_NAMESPACE::DPalette::ColorRole, qreal>> m_colorRoleMap;
};

}
#endif // THEMEDLABLE_H
