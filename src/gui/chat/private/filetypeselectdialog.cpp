#include "filetypeselectdialog.h"

#include <DFontSizeManager>
#include <DPalette>
#include <DPaletteHelper>
#include <DTitlebar>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>

#define ICON_SIZE 64
#define PAINT_PADDING 24

DWIDGET_USE_NAMESPACE

namespace uos_ai {

FileTypeSelectDialog::FileTypeSelectDialog(const QList<QIcon> &fileIcons, int fileCount, DWidget *parent)
    : DAbstractDialog(parent)
    , m_fileIcons(fileIcons)
    , m_fileCount(fileCount)
    , m_selectedCategory(EParserDocument::LocalMaterial)
{
    setDisplayPosition(DisplayPosition::Center);
    setModal(true);

    initUI();
}

EParserDocument::FileCategory FileTypeSelectDialog::getSelectedCategory() const
{
    return m_selectedCategory;
}

void FileTypeSelectDialog::initUI()
{
    setFixedSize(380, 248);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // 创建标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setIcon(QIcon(":/assets/images/uos-ai-assistant.svg"));
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 10);

    // 添加标题栏
    mainLayout->addWidget(titleBar);

    m_titleLabel = new DLabel(tr("File Upload"), this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setForegroundRole(QPalette::BrightText);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::Medium);
    mainLayout->addWidget(m_titleLabel);

    // 创建堆叠图标
    QWidget *iconWidget = createStackedIconWidget();
    mainLayout->addWidget(iconWidget, 0, Qt::AlignHCenter);

    m_buttonLayout = new QVBoxLayout();
    m_buttonLayout->setSpacing(10);
    m_buttonLayout->setContentsMargins(10, 0, 10, 0);

    // 根据文件数量决定布局
    if (m_fileCount == 1) {
        setupSingleFileMode();
    } else {
        setupMultiFileMode();
    }

    mainLayout->addStretch();
    mainLayout->addLayout(m_buttonLayout);

    setLayout(mainLayout);
}

void FileTypeSelectDialog::setupSingleFileMode()
{
    // 按钮容器
    QHBoxLayout *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);

    // 作为参考素材按钮
    m_materialBtn = new DPushButton(tr("As Material"), this);
    m_materialBtn->setFixedHeight(36);
    connect(m_materialBtn, &DPushButton::clicked, this, [this]() {
        m_selectedCategory = EParserDocument::LocalMaterial;
        emit categorySelected(m_selectedCategory);
        accept();
    });
    buttonRow->addWidget(m_materialBtn);

    // 分隔线
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedHeight(30);
    buttonRow->addWidget(separator);

    // 作为大纲文件按钮
    m_outlineBtn = new DPushButton(tr("As Outline"), this);
    m_outlineBtn->setFixedHeight(36);
    connect(m_outlineBtn, &DPushButton::clicked, this, [this]() {
        m_selectedCategory = EParserDocument::FileOutline;
        emit categorySelected(m_selectedCategory);
        accept();
    });
    buttonRow->addWidget(m_outlineBtn);

    m_buttonLayout->addLayout(buttonRow);
}

void FileTypeSelectDialog::setupMultiFileMode()
{
    // 提示文本
    m_hintLabel = new DLabel(tr("Only 1 file is supported for outline"), this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_hintLabel, DFontSizeManager::T6, QFont::Normal);
    m_buttonLayout->addWidget(m_hintLabel);

    // 作为参考素材按钮
    m_materialBtn = new DPushButton(tr("As Material"), this);
    m_materialBtn->setFixedHeight(36);
    connect(m_materialBtn, &DPushButton::clicked, this, [this]() {
        m_selectedCategory = EParserDocument::LocalMaterial;
        emit categorySelected(m_selectedCategory);
        accept();
    });

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_materialBtn);

    m_buttonLayout->addLayout(btnLayout);
}

QWidget *FileTypeSelectDialog::createStackedIconWidget()
{
    int dragIconSize = 64;
    int pixmapSize = dragIconSize + PAINT_PADDING * 2;
    DLabel *iconLabel = new DLabel(this);
    iconLabel->setFixedSize(pixmapSize, pixmapSize);

    qreal scale = qApp->devicePixelRatio();
    QPixmap pixmap(iconLabel->size() * scale);
    pixmap.setDevicePixelRatio(scale);
    pixmap.fill(Qt::transparent);

    drawIcons(&pixmap, iconLabel->rect());
    drawCount(&pixmap, iconLabel->rect());

    iconLabel->setPixmap(pixmap);

    return iconLabel;
}

void FileTypeSelectDialog::drawIcons(QPixmap *px, const QRect &rect) const
{
    QPainter painter(px);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QSize defaultIconSize = QSize(ICON_SIZE, ICON_SIZE);

    qreal offsetX = rect.width() / 2;
    qreal offsetY = rect.height() / 2;

    for (int i = m_fileIcons.length() - 1; i >= 0; --i) {
        painter.setOpacity(1.0 - i * 0.1);

        // rotate
        qreal rotate = 10.0 * (qRound((i + 1.0) / 2.0) / 2.0 + 1.0) * (i % 2 == 1 ? -1 : 1);
        if (i == 0)
            rotate = 0;
        painter.translate(offsetX, offsetY);
        painter.rotate(rotate);
        painter.translate(-offsetX, -offsetY);

        const QPixmap &px = m_fileIcons[i].pixmap(ICON_SIZE, ICON_SIZE);
        painter.drawPixmap(PAINT_PADDING, PAINT_PADDING, px);

        // reset rotation
        painter.translate(offsetX, offsetY);
        painter.rotate(-rotate);
        painter.translate(-offsetX, -offsetY);
    }
}

void FileTypeSelectDialog::drawCount(QPixmap *px, const QRect &rect) const
{
    QPainter painter(px);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制文件数量标签
    QString countText = QString::number(m_fileCount);
    int badgeSize = 24;
    int offsetX = 5;
    int offsetY = 3;
    int badgeX = rect.width() - PAINT_PADDING - badgeSize / 2 + offsetX;
    int badgeY = rect.height() - PAINT_PADDING - badgeSize / 2 + offsetY;

    // 绘制模糊阴影 (blur: 6px)
    painter.save();
    painter.setPen(Qt::NoPen);
    int blurRadius = 6;
    int shadowOffset = 4;
    for (int i = blurRadius; i >= 0; --i) {
        qreal alpha = 0.15 * (1.0 - static_cast<qreal>(i) / blurRadius) * 0.5;  // 逐渐衰减
        painter.setBrush(QColor(255, 18, 18, static_cast<int>(alpha * 255)));
        painter.drawEllipse(QPoint(badgeX, badgeY + shadowOffset),
                          badgeSize / 2 + i, badgeSize / 2 + i);
    }
    painter.restore();

    // 绘制圆形背景
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(244, 74, 74));  // 红色色背景
    painter.drawEllipse(QPoint(badgeX, badgeY), badgeSize / 2, badgeSize / 2);
    painter.restore();

    // 绘制数字
    painter.save();
    painter.setPen(Qt::white);
    QFont font = DFontSizeManager::instance()->get(DFontSizeManager::T8, QFont::Medium);
    painter.setFont(font);
    painter.drawText(QRect(badgeX - badgeSize / 2, badgeY - badgeSize / 2, badgeSize, badgeSize),
                     Qt::AlignCenter, countText);
    painter.restore();
}

} // namespace uos_ai
