// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosaiplugin.h"

#include <DGuiApplicationHelper>
#include <DApplication>
#include <QIcon>
#include <QtDBus>
#include <QPainter>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

#define QUICK_ITEM_KEY QStringLiteral("quick_item_key")
#define PLUGIN_STATE_KEY "enable"

UosAiPlugin::UosAiPlugin(QObject *parent)
    : QObject(parent)
    , m_tipsLabel(new QLabel)
#ifdef USE_DOCK_API_V2
    , m_messageCallback(nullptr)
#endif
{
    m_tipsLabel->setVisible(false);
    m_tipsLabel->setObjectName("uosai");
    m_tipsLabel->setAccessibleName("TipsLabel");
    m_tipsLabel->setAlignment(Qt::AlignCenter);

    changeTheme();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &UosAiPlugin::changeTheme);
#ifdef USE_DOCK_API_V2
    QDBusConnection::sessionBus().connect("com.deepin.copilot", "/com/deepin/copilot", "com.deepin.copilot", "windowVisibleChanged", this, SLOT(onUosAiVisibleChanged(bool)));
    QDBusConnection::sessionBus().connect("com.deepin.copilot", "/com/deepin/copilot", "com.deepin.copilot", "windowActiveChanged", this, SLOT(onUosAiVisibleChanged(bool)));
#endif
}

void UosAiPlugin::changeTheme()
{
    QPalette pa = m_tipsLabel->palette();
    pa.setBrush(QPalette::WindowText, pa.brightText());
    m_tipsLabel->setPalette(pa);
}

const QString UosAiPlugin::pluginName() const
{
    return "uosai";
}

const QString UosAiPlugin::pluginDisplayName() const
{
    return tr("UOS AI");
}

QWidget *UosAiPlugin::itemWidget(const QString &itemKey)
{
#ifdef USE_V23_DOCK
    if (itemKey == QUICK_ITEM_KEY)
        return m_quickWidget.data();
#endif

    return m_itemWidget;
}

QWidget *UosAiPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QString text = QString(tr("UOS AI"));

    m_tipsLabel->setText(text);
    const QFontMetrics &metrics = m_tipsLabel->fontMetrics();
    m_tipsLabel->setFixedSize(metrics.width(text) + 20, metrics.boundingRect(text).height());

    return m_tipsLabel;
}

void UosAiPlugin::init(PluginProxyInterface *proxyInter)
{
    QString applicationName = qApp->applicationName();
    qApp->setApplicationName("uos-ai");
    qApp->loadTranslator();
    qApp->setApplicationName(applicationName);

    m_proxyInter = proxyInter;

    m_itemWidget = new UosAiWidget;
    m_itemWidget->setAccessibleName("ItemWidget");

#ifdef USE_V23_DOCK
    if (m_quickWidget.isNull())
        m_quickWidget.reset(new QuickPanel(pluginDisplayName()));
#endif

    if (!pluginIsDisable()) {
        m_proxyInter->itemAdded(this, pluginName());
    }
}

const QString UosAiPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QDBusConnection connection = QDBusConnection::sessionBus();
    bool isServiceRegistered = connection.interface()->isServiceRegistered("com.deepin.copilot");
    if (isServiceRegistered) {
        QDBusInterface notification("com.deepin.copilot", "/com/deepin/copilot", "com.deepin.copilot", QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage").errorMessage();
        if (error.isEmpty()) {
            return "";
        }
    }
#ifdef COMPILE_ON_V23
    return "dde-am uos-ai-assistant";
#else
    return "uos-ai-assistant --chat";
#endif
}

int UosAiPlugin::itemSortKey(const QString &itemKey)
{
    Dock::DisplayMode mode = displayMode();
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(mode);
    return m_proxyInter->getValue(this, key, DOCK_DEFAULT_POS).toInt();
}

void UosAiPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(displayMode());
    m_proxyInter->saveValue(this, key, order);
}

void UosAiPlugin::pluginStateSwitched()
{
    bool pluginState = !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool();
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, pluginState);

    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        m_proxyInter->itemAdded(this, pluginName());
    }
}

bool UosAiPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool();
}

#ifdef USE_V23_DOCK
QIcon UosAiPlugin::icon(const DockPart &dockPart, DGuiApplicationHelper::ColorType themeType)
{
    QString iconName = "UosAiAssistant";
    if (dockPart == DockPart::DCCSetting) {
        QPixmap pixmap = loadSvg(iconName, QSize(18, 18));
        if (themeType == DGuiApplicationHelper::ColorType::LightType) {
            return pixmap;
        } else {
            QPainter pa(&pixmap);
            pa.setCompositionMode(QPainter::CompositionMode_SourceIn);
            pa.fillRect(pixmap.rect(), Qt::white);
            return pixmap;
        }
    }
    return QIcon();
}
#endif

#ifdef USE_DOCK_API_V2
void UosAiPlugin::onUosAiVisibleChanged(bool visible)
{
    qDebug() << "onUosAiVisibleChanged, visible: " << visible;
    if (!m_messageCallback) {
            qWarning() << "Message callback function is nullptr";
            return;
        }
        QJsonObject msg;
        msg[Dock::MSG_TYPE] = Dock::MSG_ITEM_ACTIVE_STATE;
        msg[Dock::MSG_DATA] = visible;
        QJsonDocument doc;
        doc.setObject(msg);
        m_messageCallback(this, doc.toJson());
}
#endif

QPixmap UosAiPlugin::loadSvg (QString &iconName, const QSize size, const qreal ratio)
{
    QIcon icon = QIcon::fromTheme(iconName);
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps) ? size : QSize(size * ratio));
        pixmap.setDevicePixelRatio(ratio);
        if (ratio == 1)
            return pixmap;
        if (pixmap.size().width() > size.width() * ratio)
            pixmap = pixmap.scaledToWidth(size.width() * ratio);
        if (pixmap.size().height() > size.height() * ratio)
            pixmap = pixmap.scaledToHeight(size.height() * ratio);
        return pixmap;
    }
    return QPixmap();
}
