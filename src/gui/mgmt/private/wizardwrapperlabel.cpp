#include "wizardwrapperlabel.h"
#include "wordwizard.h"
#include "util.h"

#include <DFontSizeManager>

#include <QPainter>
#include <QStyleOption>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

WizardWrapperLabel::WizardWrapperLabel(QWidget *parent)
    : DLabel(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    DFontSizeManager::instance()->bind(this, DFontSizeManager::T7, QFont::Normal);
    initFunctionButtons();
    calculateWidth();
    setAlignment(Qt::AlignCenter);
    setContentsMargins(0, 0, 0, 0);

    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WizardWrapperLabel::onThemeTypeChanged);
}

QWidget* WizardWrapperLabel::createWrapper(QWidget* parent)
{
    QWidget *wrapper = new QWidget(parent);
    wrapper->setFixedSize(560, 116);

    m_helloLabel = new DLabel(parent);

    QVBoxLayout *wrapperLayout = new QVBoxLayout(wrapper);
    wrapperLayout->setSpacing(0);
    wrapperLayout->setContentsMargins(15, 12 + m_arrowHeight, 15, 10 + m_arrowHeight);
    wrapperLayout->addWidget(this, 0, Qt::AlignCenter);
    wrapperLayout->addWidget(m_helloLabel, 0, Qt::AlignCenter);

    onThemeTypeChanged();

    return wrapper;
}

void WizardWrapperLabel::calculateWidth()
{
    QFont fixedFont = this->font();
    fixedFont.setPointSize(9); // 固定字体大小为9pt
    QFontMetrics fm(fixedFont);

    int totalWidth = 0;

    // 分隔线宽度
    totalWidth += 2 + 2 + 2 + 7; // 第一条分隔线宽度(2px) + 间距(2px) + 第二条分隔线宽度(2px) + 间距(5px)

    // 图标按钮宽度
    totalWidth += 16 + 9; // 图标宽度 + 左边距

    int visibleButtonCount = 0;
    const int maxButtonTextWidth = 70; // 按钮文本最大宽度限制
    
    for (int i = 0; i < WordWizard::kCustomFunctionList.size() && visibleButtonCount < 4; ++i) {
        const CustomFunction &func = WordWizard::kCustomFunctionList.at(i);
        if (func.isHidden) {
            continue;
        }

        QString displayName = func.isCustom ? func.name : WordWizard::getDefaultSkillName(func.defaultFunctionType);
        int textWidth = fm.horizontalAdvance(displayName);
        
        // 限制文本宽度，如果超出则按省略号处理后的宽度计算
        if (textWidth > maxButtonTextWidth) {
            QString elidedText = fm.elidedText(displayName, Qt::ElideRight, maxButtonTextWidth);
            textWidth = fm.horizontalAdvance(elidedText);
        }
        
        totalWidth += textWidth + 32 + 2; // 按钮宽度 + 间距
        visibleButtonCount++;
    }

    // 分隔线宽度
    totalWidth += 2 + 7;

    // 更多和关闭按钮
    totalWidth += 24 + 2 + 24 + 10;

    // 设置固定大小
    setFixedSize(totalWidth, fm.height() + 24);
}

WizardWrapperLabel::~WizardWrapperLabel()
{
}

void WizardWrapperLabel::setRadius(int xRadius, int yRadius)
{
    if (m_xRadius != xRadius || m_yRadius != yRadius) {
        m_xRadius = xRadius;
        m_yRadius = yRadius;
        update();
    }
}

void WizardWrapperLabel::initFunctionButtons()
{
    m_iconBtnIcon = QIcon::fromTheme("uos-ai-assistant");
    m_moreBtnIcon = QIcon::fromTheme("uos-ai-assistant_more");
    m_closeBtnIcon = QIcon::fromTheme("uos-ai-assistant_close");
}

void WizardWrapperLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    drawWizardWrapper(painter);
}

void WizardWrapperLabel::drawWizardWrapper(QPainter &painter)
{
    QRect rect = this->rect();

    QPainterPath clipPath;

    int arrowX = (m_arrowX < 0) ? rect.x() + rect.width() / 2 : rect.x() + m_arrowX;
    QPoint cornerPoint(arrowX, rect.y() + rect.height());

    clipPath.moveTo(rect.x() + m_xRadius, rect.y());
    clipPath.lineTo(rect.x() + rect.width() - m_xRadius, rect.y());
    clipPath.arcTo(rect.x() + rect.width() - 2 * m_xRadius, rect.y(), 2 * m_xRadius, 2 * m_xRadius, 90, -90);
    clipPath.lineTo(rect.x() + rect.width(), rect.y() + rect.height() - m_arrowHeight - m_yRadius);
    clipPath.arcTo(rect.x() + rect.width() - 2 * m_xRadius, rect.y() + rect.height() - 2 * m_yRadius - m_arrowHeight, 2 * m_xRadius, 2 * m_yRadius, 0, -90);

    if (m_radiusArrowStyleEnable) {
        clipPath.lineTo(cornerPoint.x() + m_arrowWidth / 2 + m_xRadius / 2, cornerPoint.y() - m_arrowHeight);

        clipPath.cubicTo(QPointF(cornerPoint.x() + m_arrowWidth / 2 + m_xRadius / 2, cornerPoint.y() - m_arrowHeight),
                         QPointF(cornerPoint.x() + m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight),
                         QPointF(cornerPoint.x() + m_arrowWidth / 4.5, cornerPoint.y() - m_arrowHeight / 2));

        clipPath.cubicTo(QPointF(cornerPoint.x() + m_arrowWidth / 4.5, cornerPoint.y() - m_arrowHeight / 2),
                         QPointF(cornerPoint),
                         QPointF(cornerPoint.x() - m_arrowWidth / 4.5, cornerPoint.y() - m_arrowHeight / 2));

        clipPath.cubicTo(QPointF(cornerPoint.x() - m_arrowWidth / 4.5, cornerPoint.y() - m_arrowHeight / 2),
                         QPointF(cornerPoint.x() - m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight),
                         QPointF(cornerPoint.x() - m_arrowWidth / 2 - m_xRadius / 2, cornerPoint.y() - m_arrowHeight));
    } else {
        clipPath.lineTo(cornerPoint.x() + m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight);
        clipPath.lineTo(cornerPoint);
        clipPath.lineTo(cornerPoint.x() - m_arrowWidth / 2, cornerPoint.y() - m_arrowHeight);
    }

    clipPath.lineTo(rect.x() + m_xRadius, rect.y() + rect.height() - m_arrowHeight);
    clipPath.arcTo(rect.x(), rect.y() + rect.height() - 2 * m_yRadius - m_arrowHeight, 2 * m_xRadius, 2 * m_yRadius, -90, -90);
    clipPath.lineTo(rect.x(), rect.y() + m_yRadius);
    clipPath.arcTo(rect.x(), rect.y(), 2 * m_xRadius, 2 * m_yRadius, 180, -90);

    painter.setClipPath(clipPath);

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);

    painter.setBrush(backgroundColor);
    painter.setPen(Qt::NoPen);
    painter.drawPath(clipPath);

    painter.fillPath(clipPath, getMaskColor());

    painter.setClipping(false);

    QColor borderColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        borderColor = QColor(0, 0, 0, 15);
    } else {
        borderColor = QColor(255, 255, 255, 20);
    }

    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(clipPath);

    drawButtons(painter, 10, (rect.height() - m_arrowHeight) / 2);
}

void WizardWrapperLabel::drawButtons(QPainter &painter, int startX, int y)
{
    QColor sepColor;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        sepColor = QColor(0, 0, 0, 25);
    } else {
        sepColor = QColor(255, 255, 255, 25);
    }

    if (!isActiveWindow()) {
        sepColor.setAlphaF(sepColor.alphaF() * 0.6);
    }

    painter.setPen(sepColor);
    painter.fillRect(QRect(startX, y - 7, 2, 15), sepColor);
    startX += 4;
    painter.fillRect(QRect(startX, y - 7, 2, 15), sepColor);
    startX += 9;

    if (!m_iconBtnIcon.isNull()) {
        QPixmap iconPixmap = m_iconBtnIcon.pixmap(QSize(16, 16));
        QPixmap iconWithAlpha = iconPixmap;
        QPainter iconPainter(&iconWithAlpha);
        iconPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        double alpha = isActiveWindow() ? 1.0 : 0.4;
        iconPainter.fillRect(iconWithAlpha.rect(), QColor(0, 0, 0, 255 * alpha));
        iconPainter.end();
        painter.drawPixmap(startX, y - 8, iconWithAlpha);
    }
    startX += 25;

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor textColor = parentPb.color(DPalette::Normal, DPalette::BrightText);

    double alphaFactor = isActiveWindow() ? 0.7 : 0.4;
    textColor.setAlphaF(alphaFactor);

    painter.setPen(textColor);

    QFont fixedFont = painter.font();
    fixedFont.setPointSize(9); // 固定字体大小为9pt
    painter.setFont(fixedFont);
    QFontMetrics fm(fixedFont);
    
    const int maxButtonTextWidth = 70; // 按钮文本最大宽度限制

    int visibleButtonCount = 0;
    for (int i = 0; i < WordWizard::kCustomFunctionList.size() && visibleButtonCount < 4; ++i) {
        const CustomFunction &func = WordWizard::kCustomFunctionList.at(i);
        if (func.isHidden) {
            continue;
        }

        QIcon buttonIcon;
        QString displayName;
        
        if (func.isCustom) {
            buttonIcon = QIcon::fromTheme("uos-ai-assistant_custom");
            displayName = func.name;
        } else {
            buttonIcon = WordWizard::getDefaultSkillIcon(func.defaultFunctionType);
            displayName = WordWizard::getDefaultSkillName(func.defaultFunctionType);
        }

        if (!buttonIcon.isNull()) {
            QPixmap iconPixmap = buttonIcon.pixmap(QSize(16, 16));
            QPixmap iconWithAlpha = iconPixmap;
            QPainter iconPainter(&iconWithAlpha);
            iconPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            iconPainter.fillRect(iconWithAlpha.rect(), QColor(0, 0, 0, 255 * alphaFactor));
            iconPainter.end();
            painter.drawPixmap(startX, y - 8, iconWithAlpha);
        }

        // 限制文本宽度，如果超出则使用省略号处理
        QString elidedDisplayName = displayName;
        int textWidth = fm.horizontalAdvance(displayName);
        if (textWidth > maxButtonTextWidth) {
            elidedDisplayName = fm.elidedText(displayName, Qt::ElideRight, maxButtonTextWidth);
            textWidth = fm.horizontalAdvance(elidedDisplayName);
        }
        
        int buttonWidth = textWidth + 32;
        QRectF textRect(startX + 20, y - fm.height()/2, textWidth, fm.height());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedDisplayName);

        startX += buttonWidth + 2;
        visibleButtonCount++;
    }

    painter.fillRect(QRect(startX, y - 7, 2, 15), sepColor);
    startX += 7;

    painter.setPen(textColor);

    if (!m_moreBtnIcon.isNull()) {
        QPixmap morePixmap = m_moreBtnIcon.pixmap(QSize(20, 20));
        QPixmap moreWithAlpha = morePixmap;
        QPainter morePainter(&moreWithAlpha);
        morePainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        morePainter.fillRect(moreWithAlpha.rect(), QColor(0, 0, 0, 255 * alphaFactor));
        morePainter.end();
        painter.drawPixmap(startX, y - 10, moreWithAlpha);
    }
    startX += 25;

    if (!m_closeBtnIcon.isNull()) {
        QPixmap closePixmap = m_closeBtnIcon.pixmap(QSize(20, 20));
        QPixmap closeWithAlpha = closePixmap;
        QPainter closePainter(&closeWithAlpha);
        closePainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        closePainter.fillRect(closeWithAlpha.rect(), QColor(0, 0, 0, 255 * alphaFactor));
        closePainter.end();
        painter.drawPixmap(startX, y - 10, closeWithAlpha);
    }
}

QColor WizardWrapperLabel::getMaskColor() const
{
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        backgroundColor.setAlphaF(isActiveWindow() ? 0.5 : 0.3);
    } else {
        backgroundColor.setAlphaF(isActiveWindow() ? 0.5 : 0.3);
    }

    return backgroundColor;
}

void WizardWrapperLabel::onUpdateSystemFont(const QFont &)
{
    calculateWidth();
    update();
}

void WizardWrapperLabel::onThemeTypeChanged()
{
    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    m_backgroundColor = parentPb.color(DPalette::Normal, DPalette::NColorTypes);

    m_textColor = parentPb.color(DPalette::Normal, DPalette::BrightText);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        m_maskAlpha = 0.3;

        QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setOffset(0, 2);
        shadowEffect->setBlurRadius(15);
        shadowEffect->setColor(QColor(0, 0, 0, 80));
        setGraphicsEffect(shadowEffect);
    } else {
        m_maskAlpha = 0.5;

        QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setOffset(0, 2);
        shadowEffect->setBlurRadius(15);
        shadowEffect->setColor(QColor(0, 0, 0, 120));
        setGraphicsEffect(shadowEffect);
    }

    QString iconName;
    QSize iconSize;
    if (Util::checkLanguage()) {
        iconName = "uos-ai-assistant_hello";
        iconSize.scale(40, 24, Qt::IgnoreAspectRatio);
    }
    else {
        iconName = "uos-ai-assistant_hello_en";
        iconSize.scale(48, 24, Qt::IgnoreAspectRatio);
    }
    m_helloLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(iconSize));

    update();
}

void WizardWrapperLabel::onSkillListChanged()
{
    calculateWidth();
    update();
}
