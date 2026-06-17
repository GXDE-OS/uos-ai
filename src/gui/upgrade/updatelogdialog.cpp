#include "updatelogdialog.h"
#include "utils/util.h"
#include "global_define.h"

#include <DFontSizeManager>
#include <DTitlebar>
#include <DFrame>
#include <DPaletteHelper>
#include <DWindowCloseButton>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QLocale>
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

inline constexpr int kDescriptionImageMaxWidth = 480;
inline constexpr char kUpdateLogResourceDir[] = ":/assets/updatelog/";
inline constexpr char kUpdateLogAssetDir[] = "assets/updatelog/";
inline constexpr int kMaxUpdateLogVersionCount = 10;

inline constexpr int kVersionCardLightBackgroundAlpha = 102; // 255 * 0.4
inline constexpr int kVersionCardDarkBackgroundAlpha = 77;   // 255 * 0.3
inline constexpr int kVersionCardLightShadowAlpha = 26;      // 255 * 0.1
inline constexpr int kVersionCardDarkShadowAlpha = 102;       // 255 * 0.4
inline constexpr int kVersionCardRadius = 6;

UpdateLogVersionCard::UpdateLogVersionCard(QWidget *parent)
    : DFrame(parent)
{
    setAutoFillBackground(false);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, [this](DGuiApplicationHelper::ColorType) { update(); });
}

void UpdateLogVersionCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    const bool isDarkTheme = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
    const QColor backgroundColor = isDarkTheme ? QColor(0, 0, 0, kVersionCardDarkBackgroundAlpha)
                                               : QColor(255, 255, 255, kVersionCardLightBackgroundAlpha);
    const QColor shadowColor = isDarkTheme ? QColor(0, 0, 0, kVersionCardDarkShadowAlpha)
                                           : QColor(0, 0, 0, kVersionCardLightShadowAlpha);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    const QRectF backgroundRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(backgroundRect, kVersionCardRadius, kVersionCardRadius);

    QPainterPath shadowPath;
    shadowPath.addRoundedRect(backgroundRect.translated(0, 1), kVersionCardRadius, kVersionCardRadius);
    shadowPath = shadowPath.subtracted(backgroundPath);

    painter.setBrush(shadowColor);
    painter.drawPath(shadowPath);

    painter.setBrush(backgroundColor);
    painter.drawPath(backgroundPath);
}

UpdateLogDialog::UpdateLogDialog(QWidget *parent)
    : DDialog(parent)
{
    setModal(true);
    loadUpdateLogs();
    initUI();
    initConnect();
}

void UpdateLogDialog::initUI()
{
    setFixedSize(620, 490);
    setDisplayPosition(DisplayPosition::Center);
    QIcon titleIcon(QIcon::fromTheme(getApplicationIconName()));
    setIcon(titleIcon);
    setContentLayoutContentsMargins(QMargins(0, 0, 0, 0));

    if (auto titlebar = findChild<DTitlebar*>()) {
        titlebar->setFixedHeight(40);
    }

    if (auto closeBtn = findChild<DWindowCloseButton*>()) {
        closeBtn->setFixedSize(40, 40);
    }

    if (auto icon = findChild<DIconButton*>()) {
        icon->setFixedSize(20, 20);
        icon->setIconSize(QSize(20, 20));
    }

    DLabel *titleLabel = new DLabel(tr("UOS AI Assistant Update Log"), this);
    titleLabel->setContentsMargins(0, 0, 0, 20);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setForegroundRole(QPalette::BrightText);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T5, QFont::DemiBold);

    addContent(titleLabel, Qt::AlignTop);

    m_contentWidget = new DWidget(this);
    m_contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_contentWidget->setAutoFillBackground(false);

    m_contentLayout = new QVBoxLayout;
    m_contentLayout->setContentsMargins(50, 0, 50, 0);
    m_contentLayout->setSpacing(10);

    m_scrollArea = new DScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setContentsMargins(0, 0, 0, 0);

    QPalette pal = m_scrollArea->palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    m_scrollArea->setPalette(pal);
    m_scrollArea->viewport()->setPalette(pal);

    // Add update logs
    for (const auto &version : m_updateLogs) {
        DFrame *versionCard = new UpdateLogVersionCard(m_contentWidget);
        versionCard->setFrameShape(QFrame::NoFrame);
        versionCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QVBoxLayout *versionLayout = new QVBoxLayout(versionCard);
        versionLayout->setContentsMargins(20, 10, 20, 10);
        versionLayout->setSpacing(0);

        // version
        QHBoxLayout *versionInfo = createVersionWidget(version);
        versionLayout->addLayout(versionInfo);

        // Items
        for (int i = 0; i < version.items.size(); ++i) {
            QHBoxLayout *itemWidget = createItemWidget(version.items[i], i + 1);  // 传递序号(从1开始)
            versionLayout->addLayout(itemWidget);
        }

        m_contentLayout->addWidget(versionCard);
    }

    // add stretch
    DWidget *stretchWidget = new DWidget(this);
    stretchWidget->setContentsMargins(5, 8, 5, 8);
    m_contentLayout->addWidget(stretchWidget, 1);

    m_contentWidget->setLayout(m_contentLayout);
    m_scrollArea->setWidget(m_contentWidget);

    addContent(m_scrollArea);

    // 清理dtk自带margin
    for (QObject *childObj : layout()->children()) {
        if (auto childLay = dynamic_cast<QHBoxLayout *>(childObj)) {
            childLay->setContentsMargins(0, 0, 0, 0);
        }
    }
}

void UpdateLogDialog::initConnect()
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &UpdateLogDialog::onUpdateSystemTheme);
}

void UpdateLogDialog::loadUpdateLogs()
{
    QString filename;
    if (Util::checkLanguage()) {
        filename = ":/assets/updatelog/updatelog-zh_CN.json";
    } else {
        filename = ":/assets/updatelog/updatelog-en_US.json";
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open updatelog file:" << filename;
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isArray()) {
        qWarning() << "Invalid updatelog format";
        return;
    }

    QJsonArray versionArray = doc.array();
    const int versionCount = qMin(versionArray.size(), kMaxUpdateLogVersionCount);
    for (int i = 0; i < versionCount; ++i) {
        const QJsonValue &versionValue = versionArray.at(i);
        QJsonObject versionObj = versionValue.toObject();

        UpdateLogVersion version;
        version.version = versionObj["version"].toString();
#ifdef COMPILE_ON_V25
        version.date = versionObj["date-for-v25"].toString();
#else
        version.date = versionObj["date-for-v20"].toString();
#endif

        QJsonArray itemsArray = versionObj["items"].toArray();
        for (const QJsonValue &itemValue : itemsArray) {
            QJsonObject itemObj = itemValue.toObject();
            UpdateLogItem item;
            item.type = itemObj["type"].toString();
            parseDescription(itemObj["description"], &item.description, &item.descriptionImages);
            item.descriptionImages.append(parseDescriptionImages(itemObj["image"]));
            item.descriptionImages.append(parseDescriptionImages(itemObj["images"]));
            version.items.append(item);
        }

        m_updateLogs.append(version);
    }
}

bool UpdateLogDialog::isSupportedDescriptionImage(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("png")
            || suffix == QLatin1String("jpg")
            || suffix == QLatin1String("jpeg");
}

QString UpdateLogDialog::updateLogImagePath(const QString &path)
{
    const QString trimmedPath = path.trimmed();
    if (trimmedPath.startsWith(QLatin1String(":/")) || trimmedPath.startsWith(QLatin1Char('/'))) {
        return trimmedPath;
    }

    if (trimmedPath.startsWith(kUpdateLogAssetDir)) {
        return QStringLiteral(":/") + trimmedPath;
    }

    const QString parentRelativeAssetDir = QStringLiteral("../") + kUpdateLogAssetDir;
    if (trimmedPath.startsWith(parentRelativeAssetDir)) {
        return QStringLiteral(":/") + trimmedPath.mid(3);
    }

    return kUpdateLogResourceDir + trimmedPath;
}

QStringList UpdateLogDialog::parseDescriptionImages(const QJsonValue &value)
{
    QStringList images;
    auto appendImage = [&images](const QString &imagePath) {
        const QString trimmedPath = imagePath.trimmed();
        if (trimmedPath.isEmpty()) {
            return;
        }

        if (!isSupportedDescriptionImage(trimmedPath)) {
            qWarning() << "Unsupported updatelog description image type:" << trimmedPath;
            return;
        }

        images.append(updateLogImagePath(trimmedPath));
    };

    if (value.isString()) {
        appendImage(value.toString());
    } else if (value.isArray()) {
        const QJsonArray imageArray = value.toArray();
        for (const QJsonValue &imageValue : imageArray) {
            appendImage(imageValue.toString());
        }
    }

    return images;
}

void UpdateLogDialog::parseDescription(const QJsonValue &descriptionValue, QString *description, QStringList *images)
{
    if (descriptionValue.isString()) {
        *description = descriptionValue.toString();
        return;
    }

    if (descriptionValue.isObject()) {
        const QJsonObject descriptionObj = descriptionValue.toObject();
        *description = descriptionObj["text"].toString();
        images->append(parseDescriptionImages(descriptionObj["image"]));
        images->append(parseDescriptionImages(descriptionObj["images"]));
        return;
    }

    if (descriptionValue.isArray()) {
        const QJsonArray descriptionArray = descriptionValue.toArray();
        for (const QJsonValue &partValue : descriptionArray) {
            if (partValue.isString()) {
                if (!description->isEmpty()) {
                    description->append(QLatin1Char('\n'));
                }
                description->append(partValue.toString());
                continue;
            }

            if (!partValue.isObject()) {
                continue;
            }

            const QJsonObject partObj = partValue.toObject();
            const QString partType = partObj["type"].toString();
            if (partType == QLatin1String("image")) {
                images->append(parseDescriptionImages(partObj["path"]));
                images->append(parseDescriptionImages(partObj["src"]));
            } else {
                const QString text = partObj["text"].toString();
                if (!text.isEmpty()) {
                    if (!description->isEmpty()) {
                        description->append(QLatin1Char('\n'));
                    }
                    description->append(text);
                }
            }
        }
    }
}

QHBoxLayout* UpdateLogDialog::createVersionWidget(const UpdateLogVersion &version)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(0);

    DLabel *versionLabel = new DLabel(version.version);
    versionLabel->setContentsMargins(0, 0, 0, 0);
    versionLabel->setObjectName("VersionLabel");
    versionLabel->setForegroundRole(QPalette::BrightText);
    DFontSizeManager::instance()->bind(versionLabel, DFontSizeManager::T5, QFont::Medium);

    DLabel *dateLabel = new DLabel(version.date);
    dateLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    dateLabel->setContentsMargins(0, 0, 0, 0);
    dateLabel->setObjectName("DateLabel");
    dateLabel->setDisabled(true);
    DFontSizeManager::instance()->bind(dateLabel, DFontSizeManager::T8, QFont::Normal);

    layout->addWidget(versionLabel);
    layout->addStretch(10);
    layout->addWidget(dateLabel, 1);

    return layout;
}

QHBoxLayout* UpdateLogDialog::createItemWidget(const UpdateLogItem &item, int index)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    DWidget *itemContent = new DWidget(m_contentWidget);
    QVBoxLayout *itemContentLayout = new QVBoxLayout(itemContent);
    itemContentLayout->setContentsMargins(0, 0, 0, 0);
    itemContentLayout->setSpacing(8);

    // Description
    DLabel *descLabel = new DLabel(QString::number(index) + "." + item.description);
    descLabel->setContentsMargins(0, 8, 0, 8);
    descLabel->setObjectName("DescLabel");
    descLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T6, QFont::Thin);

    itemContentLayout->addWidget(descLabel);

    for (const QString &imagePath : item.descriptionImages) {
        QPixmap image(imagePath);
        if (image.isNull()) {
            qWarning() << "Failed to load updatelog description image:" << imagePath;
            continue;
        }

        DLabel *imageLabel = new DLabel(itemContent);
        imageLabel->setObjectName("DescImageLabel");
        imageLabel->setContentsMargins(0, 0, 0, 8);
        imageLabel->setAlignment(Qt::AlignLeft);

        const int targetWidth = qMin(kDescriptionImageMaxWidth, image.width());
        imageLabel->setPixmap(image.scaledToWidth(targetWidth, Qt::SmoothTransformation));
        itemContentLayout->addWidget(imageLabel);
    }

    layout->addWidget(itemContent, 1);

    return layout;
}

void UpdateLogDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    // Update theme-dependent styles if needed
    // The DTK widgets will handle most theme changes automatically
}
