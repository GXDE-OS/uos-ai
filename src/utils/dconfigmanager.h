// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DCONFIGMANAGER_H
#define DCONFIGMANAGER_H

#include <uosai_global.h>

#include <QObject>
#include <QVariant>

namespace  uos_ai {
#define APP_GROUP "uos-ai-assistant"
#define ISFIRSTDCONFIG "isFirstDconfig"

#define WORDWIZARD_GROUP "uos-ai-assistant.wordwizard"
#define WORDWIZARD_ISFIRSTCLOSE "isFirstClose"
#define WORDWIZARD_ISHIDDEN "isHidden"
#define WORDWIZARD_DISABLED_APPS "disabledApps"
#define WORDWIZARD_CUSTOM_FUNCTIONS "customFunctions"
#define WORDWIZARD_LABELTOOLTIP "isTooltipFirstShow"
#define WORDWIZARD_ISFIRSTCLICKEDASKAI "isFirstClickAskAI"

#define AIBAR_GROUP "uos-ai-assistant.aibar"
#define AIBAR_ENABLEFILEDRAG "enableFileDrag"

#define AIQUICK_GROUP "uos-ai-assistant.aiquick"
#define AIQUICK_ISFIRSTFILL "isFirstFill"
#define AIQUICK_ISFIRSTTRANSLATE "isFirstTranslate"
#define AIQUICK_TRANSLATE_TARGET_LANGUAGE "translateTargetLanguage"

#define AUDIOWIZARD_GROUP "uos-ai-assistant.audiowizard"
#define AUDIOWIZARD_IAT_NOSPEAK_DELAY_MS "iatNoSpeakDelayMs"

#define TTS_GROUP "uos-ai-assistant.tts"
#define IAT_GROUP "uos-ai-assistant.iat"

#define LLM_GROUP "uos-ai-assistant.llm"
#define LLM_UOSAIRAG "aiRagVersion"

#define MCP_GROUP "uos-ai-assistant.mcp"
#define MCP_ENABLED_LIST "enabledList"

class DConfigManagerPrivate;
class DConfigManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DConfigManager)

public:
    static DConfigManager *instance();

    bool addConfig(const QString &config, QString *err = nullptr);
    bool removeConfig(const QString &config, QString *err = nullptr);

    QStringList keys(const QString &config) const;
    bool contains(const QString &config, const QString &key) const;
    QVariant value(const QString &config, const QString &key, const QVariant &fallback = QVariant()) const;
    void setValue(const QString &config, const QString &key, const QVariant &value);

    bool validateConfigs(QStringList &invalidConfigs) const;

    void configMigrate();

    static bool checkConfigAvailable(const QString &config, const QString &key);

Q_SIGNALS:
    void valueChanged(const QString &config, const QString &key);

private:
    explicit DConfigManager(QObject *parent = nullptr);
    virtual ~DConfigManager() override;

private:
    QScopedPointer<DConfigManagerPrivate> d;
};
}

#endif   // DCONFIGMANAGER_H
