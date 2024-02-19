// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSAIPLUGIN_H
#define UOSAIPLUGIN_H

#include "uosaiwidget.h"

#include <QApplication>
#include <QDBusInterface>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>
#include <pluginsiteminterface.h>

#define DOCK_DEFAULT_POS    9
#define KEY_DNDMODE         0
#define KEY_SHOWICON        5

#ifdef DOCK_MIN_SIZE
#define USE_V23_DOCK
#endif

class QGSettings;
class UosAiPlugin : public QObject, PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "uosai.json")

public:
    explicit UosAiPlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    const QString itemCommand(const QString &itemKey) override;
    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;

    void pluginStateSwitched() override;
    bool pluginIsAllowDisable() override { return true; }
    bool pluginIsDisable() override;
#ifdef USE_V23_DOCK
    QIcon icon(const DockPart &dockPart, DGuiApplicationHelper::ColorType themeType) override;
#endif

private:
    const QPixmap loadSvg(const QString &iconName, const QSize size, const qreal ratio = qApp->devicePixelRatio());

private slots:
    void changeTheme();
private:
    void loadPlugin();

private:
    bool m_pluginLoaded = false;
    UosAiWidget *m_itemWidget = nullptr;
    QLabel *m_tipsLabel;
};

#endif // UOSAIPLUGIN_H
