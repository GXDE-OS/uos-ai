#include "updatelogdialog.h"
#include "utils/util.h"

#include <DFontSizeManager>
#include <DTitlebar>
#include <DFrame>
#include <DPaletteHelper>

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

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

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
    setFixedSize(700, 540);
    setDisplayPosition(DisplayPosition::Center);
    setAttribute(Qt::WA_TranslucentBackground, true);
    QIcon titleIcon(":/assets/images/uos-ai-assistant.svg");
    setIcon(titleIcon);

    DLabel *titleLabel = new DLabel(tr("UOS AI Assistant Update Log"), this);
    titleLabel->setContentsMargins(0, 0, 0, 10);
    titleLabel->setAlignment(Qt::AlignCenter);
    QPalette titleLabelPa = titleLabel->palette();
    titleLabelPa.setColor(QPalette::WindowText, titleLabelPa.color(QPalette::BrightText));
    titleLabel->setPalette(titleLabelPa);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T6, QFont::DemiBold);

    addContent(titleLabel, Qt::AlignTop);

    m_contentWidget = new DWidget(this);
    m_contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_contentWidget->setAutoFillBackground(false);
    m_contentWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_contentLayout = new QVBoxLayout;
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);

    m_scrollArea = new DScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    // Add update logs
    for (const auto &version : m_updateLogs) {
        // version
        QHBoxLayout *versionInfo = createVersionWidget(version);
        m_contentLayout->addLayout(versionInfo);

        // Items
        for (int i = 0; i < version.items.size(); ++i) {
            QHBoxLayout *itemWidget = createItemWidget(version.items[i], i + 1);  // 传递序号(从1开始)
            m_contentLayout->addLayout(itemWidget);
        }

        // Add separator line
        DFrame *separator = new DFrame();
        separator->setContentsMargins(60, 8, 60, 8);
        separator->setFrameShape(QFrame::HLine);
        separator->setObjectName("Separator");
        m_contentLayout->addWidget(separator);
    }

    // add stretch
    DWidget *stretchWidget = new DWidget(this);
    stretchWidget->setContentsMargins(5, 8, 5, 8);
    m_contentLayout->addWidget(stretchWidget, 1);

    m_contentWidget->setLayout(m_contentLayout);
    m_scrollArea->setWidget(m_contentWidget);

    addContent(m_scrollArea);
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
    for (const QJsonValue &versionValue : versionArray) {
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
            item.description = itemObj["description"].toString();
            version.items.append(item);
        }

        m_updateLogs.append(version);
    }
}

QHBoxLayout* UpdateLogDialog::createVersionWidget(const UpdateLogVersion &version)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(0);

    DLabel *versionLabel = new DLabel(version.version);
    versionLabel->setContentsMargins(60, 8, 5, 8);
    versionLabel->setObjectName("VersionLabel");
    QPalette versionLabelPa = versionLabel->palette();
    versionLabelPa.setColor(QPalette::WindowText, versionLabelPa.color(QPalette::BrightText));
    versionLabel->setPalette(versionLabelPa);
    DFontSizeManager::instance()->bind(versionLabel, DFontSizeManager::T6, QFont::DemiBold);

    DLabel *dateLabel = new DLabel(version.date);
    dateLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    dateLabel->setContentsMargins(5, 8, 5, 8);
    dateLabel->setObjectName("DateLabel");
    dateLabel->setDisabled(true);
    DFontSizeManager::instance()->bind(dateLabel, DFontSizeManager::T8, QFont::Normal);

    layout->addWidget(versionLabel);
    layout->addWidget(dateLabel, 1);

    return layout;
}

QHBoxLayout* UpdateLogDialog::createItemWidget(const UpdateLogItem &item, int index)
{
    QHBoxLayout *layout = new QHBoxLayout;

    // Description
    DLabel *descLabel = new DLabel(QString::number(index) + "." + item.description);
    descLabel->setContentsMargins(60, 8, 60, 8);
    descLabel->setObjectName("DescLabel");
    descLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T6, QFont::Normal);

    layout->addWidget(descLabel, 1);

    return layout;
}

void UpdateLogDialog::onUpdateSystemTheme(const DGuiApplicationHelper::ColorType &)
{
    // Update theme-dependent styles if needed
    // The DTK widgets will handle most theme changes automatically
}
