// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dconfigmanager.h"
#include "private/dconfigmanager_p.h"

#include <DConfig>
#include <DDialog>

#include <QDebug>
#include <QSet>
#include <QStandardPaths>
#include <QSettings>
#include <QGuiApplication>
#include <QFileInfo>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QIcon>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logUtils)

static constexpr char kCfgAppId[] { "uos-ai-assistant" };

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

DConfigManager::DConfigManager(QObject *parent)
    : QObject(parent), d(new DConfigManagerPrivate(this))
{
    addConfig(WORDWIZARD_GROUP);
    addConfig(AIQUICK_GROUP);
    addConfig(AUDIOWIZARD_GROUP);
    addConfig(TTS_GROUP);
    addConfig(IAT_GROUP);
    addConfig(LLM_GROUP);
    addConfig(MCP_GROUP);
#ifdef ENABLE_AI_BAR
    addConfig(AIBAR_GROUP);
#endif

    this->configMigrate();
}

DConfigManager *DConfigManager::instance()
{
    static DConfigManager ins;
    return &ins;
}

DConfigManager::~DConfigManager()
{
#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    auto configs = d->configs.values();
    std::for_each(configs.begin(), configs.end(), [](DConfig *cfg) { delete cfg; });
    d->configs.clear();
#endif
}

bool DConfigManager::addConfig(const QString &config, QString *err)
{
#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        if (err)
            *err = "config is already added";
        return false;
    }

    auto cfg = DConfig::create(kCfgAppId, config, "", this);
    if (!cfg) {
        if (err)
            *err = "cannot create config";
        return false;
    }

    if (!cfg->isValid()) {
        if (err)
            *err = "config is not valid";
        delete cfg;
        return false;
    }

    d->configs.insert(config, cfg);
    locker.unlock();
    connect(cfg, &DConfig::valueChanged, this, [=](const QString &key) { Q_EMIT valueChanged(config, key); });
#endif
    return true;
}

bool DConfigManager::removeConfig(const QString &config, QString *err)
{
#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        delete d->configs[config];
        d->configs.remove(config);
    }
#endif
    return true;
}

QStringList DConfigManager::keys(const QString &config) const
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (!d->configs.contains(config))
        return QStringList();

    return d->configs[config]->keyList();
#else
    return QStringList();
#endif
}

bool DConfigManager::contains(const QString &config, const QString &key) const
{
    return key.isEmpty() ? false : keys(config).contains(key);
}

QVariant DConfigManager::value(const QString &config, const QString &key, const QVariant &fallback) const
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        // 如果DConfig没刷新，存在有config但没key的情况
        if (!this->contains(config, key)) {
            qCWarning(logUtils) << "DConfig has no key:" << key << "in config:" << config << "REBOOT may fix it";
            return fallback;
        }

        //qDebug() << "dconfig:" << config << "key:" << key << "value:" << d->configs.value(config)->value(key, fallback);
        return d->configs.value(config)->value(key, fallback);
    }
    else
        qCWarning(logUtils) << "Config:" << config << "is not registered!!!";
    return fallback;
#else
    return fallback;
#endif
}

void DConfigManager::setValue(const QString &config, const QString &key, const QVariant &value)
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        // 如果DConfig没刷新，存在有config但没key的情况
        if (!this->contains(config, key)) {
            qCWarning(logUtils) << "DConfig has no key:" << key << "in config:" << config << "REBOOT may fix it";
            return;
        }

        qCInfo(logUtils) << "dconfig:" << config << "key:" << key << "value:" << value;
        d->configs.value(config)->setValue(key, value);
    } else {
        qCWarning(logUtils) << "Config:" << config << "is not registered!!!";
    }
#endif
}

bool DConfigManager::validateConfigs(QStringList &invalidConfigs) const
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    bool ret = true;
    for (auto iter = d->configs.cbegin(); iter != d->configs.cend(); ++iter) {
        bool valid = iter.value()->isValid();
        if (!valid)
            invalidConfigs << iter.key();
        ret &= valid;
    }
    return ret;
#else
    return true;
#endif
}

void DConfigManager::configMigrate()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    configPath = configPath
                 + "/" + qApp->organizationName()
                 + "/" + qApp->applicationName()
                 + "/" + qApp->applicationName() + ".conf";

    QFileInfo configFile(configPath);
    if (!configFile.exists()) {
        return;
    }

    QSettings set(configPath, QSettings::IniFormat);
    bool isFirstDconfig = false;
    set.beginGroup("dconfig");
    isFirstDconfig = set.value(ISFIRSTDCONFIG, true).toBool();
    if (isFirstDconfig) {
        set.setValue(ISFIRSTDCONFIG, false);
    }
    set.endGroup();
    if (!isFirstDconfig) {
        return;
    }

    qCDebug(logUtils) << "First use dconfig, migrate old config START";
    set.beginGroup("WordWizard");
    this->setValue(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLOSE, set.value(WORDWIZARD_ISFIRSTCLOSE, true).toBool());
    this->setValue(WORDWIZARD_GROUP, WORDWIZARD_ISHIDDEN, set.value(WORDWIZARD_ISHIDDEN, false).toBool());
    this->setValue(WORDWIZARD_GROUP, WORDWIZARD_ISFIRSTCLICKEDASKAI, set.value(WORDWIZARD_ISFIRSTCLICKEDASKAI, true).toBool());
    set.endGroup();

    set.beginGroup("aiquick");
    this->setValue(AIQUICK_GROUP, AIQUICK_ISFIRSTFILL, !set.value("notfirstfill", false).toBool());
    set.endGroup();
    qCDebug(logUtils) << "First use dconfig, migrate old config END";
}

bool DConfigManager::checkConfigAvailable(const QString &config, const QString &key)
{
    if (DConfigManager::instance()->contains(config, key)) {
        return true;
    }

    DDialog dialog(nullptr);
    dialog.setMessage(tr("The device needs to be restarted before this function can be fully used."));
    dialog.setIcon(QIcon(":/assets/images/warning.svg"));

    dialog.addButton(QObject::tr("Cancel"), true, DDialog::ButtonNormal);
    dialog.addButton(QObject::tr("Restart immediately"), true, DDialog::ButtonWarning);
    
    if (dialog.exec() == DDialog::Accepted) {
        QDBusInterface shutdownInterface("com.deepin.dde.shutdownFront",
                                        "/com/deepin/dde/shutdownFront",
                                        "com.deepin.dde.shutdownFront",
                                        QDBusConnection::sessionBus());
        
        if (shutdownInterface.isValid()) {
            shutdownInterface.call("Restart");
        }
    }
    return false;
}
