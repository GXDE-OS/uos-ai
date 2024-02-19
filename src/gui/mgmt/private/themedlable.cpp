#include "themedlable.h"

#include <DGuiApplicationHelper>

ThemedLable::ThemedLable(QWidget *parent, Qt::WindowFlags f) : DLabel(parent, f)
{
    setForegroundRole(QPalette::Text);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ThemedLable::onThemeTypeChanged);
}

ThemedLable::ThemedLable(const QString &text, QWidget *parent) : DLabel(text, parent)
{
    setForegroundRole(QPalette::Text);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ThemedLable::onThemeTypeChanged);
}

void ThemedLable::setPaletteColor(DPalette::ColorRole role, DPalette::ColorType type, qreal alphaF)
{
    m_colorRoleMap.remove(role);
    m_colorTypeMap.insert(role, std::pair<DPalette::ColorType, qreal>(type, alphaF));

    DPalette pl = this->palette();
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(type);
    color.setAlphaF(alphaF);
    pl.setColor(role, color);
    this->setPalette(pl);
}

void ThemedLable::setPaletteColor(DPalette::ColorRole role, DPalette::ColorRole role1, qreal alphaF)
{
    m_colorTypeMap.remove(role);
    m_colorRoleMap.insert(role, std::pair<DPalette::ColorRole, qreal>(role1, alphaF));

    DPalette pl = this->palette();
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(role1);
    color.setAlphaF(alphaF);
    pl.setColor(role, color);
    this->setPalette(pl);
}

void ThemedLable::onThemeTypeChanged()
{
    DPalette pl = this->palette();
    for (auto t : m_colorTypeMap.keys()) {
        QColor color =  DGuiApplicationHelper::instance()->applicationPalette().color(m_colorTypeMap.value(t).first);
        color.setAlphaF(m_colorTypeMap.value(t).second);
        pl.setColor(t, color);
    }
    for (auto t : m_colorRoleMap.keys()) {
        QColor color =  DGuiApplicationHelper::instance()->applicationPalette().color(m_colorRoleMap.value(t).first);
        color.setAlphaF(m_colorRoleMap.value(t).second);
        pl.setColor(t, color);
    }
    this->setPalette(pl);
    setForegroundRole(QPalette::Text);
}
