#include "updatebutton.h"

#include <DPalette>
#include <DGuiApplicationHelper>
#include <DStylePainter>
#include <DStyleOptionButton>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QStylePainter>
#include <QLoggingCategory>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

UpdateButton::UpdateButton( QWidget * parent)
    : DSuggestButton(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
}

void UpdateButton::paintEvent(QPaintEvent* e)
{
    QPainter p(this);

    if (m_suggest) {
        DStyleOptionButton option;
        initStyleOption(&option);
        option.init(this);
        option.features |= QStyleOptionButton::ButtonFeature(DStyleOptionButton::SuggestButton);

        QColor startColor = palette().color(QPalette::Highlight);
        QColor endColor = DGuiApplicationHelper::adjustColor(startColor, 0, 0, +10, 0, 0, 0, 0);

        option.palette.setBrush(QPalette::Light, QBrush(endColor));
        option.palette.setBrush(QPalette::Dark, QBrush(startColor));
        option.palette.setBrush(QPalette::ButtonText, option.palette.highlightedText());
        style()->drawControl(QStyle::CE_PushButton, &option, &p, this);
    } else {
        QStyleOptionButton option;
        initStyleOption(&option);
        style()->drawControl(QStyle::CE_PushButton, &option, &p, this);
    }

    if (m_suggest && m_redpoint) {
        DPalette pa = DGuiApplicationHelper::instance()->standardPalette(DGuiApplicationHelper::LightType);
        //红点半径
        const int redPointRadius = 3;
        int redPointPadding = 4 + redPointRadius;
        p.setPen(pa.color(DPalette::TextWarning));
        p.setBrush(pa.color(DPalette::TextWarning));
        p.setRenderHint(QPainter::Antialiasing);
        p.drawEllipse(QPointF(size().width()-redPointPadding, redPointPadding), redPointRadius, redPointRadius);
    }
}

void UpdateButton::mousePressEvent(QMouseEvent *event)
{
    return DSuggestButton::mousePressEvent(event);
}

void UpdateButton::mouseReleaseEvent(QMouseEvent *event)
{
    DSuggestButton::mouseReleaseEvent(event);

    sigButtonclicked(m_suggest);
    if (event->button() == Qt::LeftButton) {
        qCDebug(logAIGUI) << "Update button clicked, changing status from" << m_suggest << "to" << !m_suggest;
        setStatus(!m_suggest);
    }
}

void UpdateButton::setStatus(bool isUpdate)
{
    qCInfo(logAIGUI) << "Setting update button status to:" << (isUpdate ? "Update" : "Cancel Update");
    m_suggest = isUpdate;
    setText(isUpdate ? tr("Update") : tr("Cancel Update"));
    adjustSize();
}
