#include "aiassistantsetting.h"

#include <QSettings>
#include <QApplication>
#include <QThread>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

UOSAI_USE_NAMESPACE

AiassistantSetting::AiassistantSetting(QObject *parent) : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    auto configPath = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first();
    configPath = configPath
                 + "/deepin"
                 + "/uos-ai-assistant"
                 + "/assistantsubstitute.conf";

    QFileInfo configFile(configPath);
    if (!configFile.exists())
        configFile.absoluteDir().mkpath(".");

    m_setting = new QSettings(configPath, QSettings::IniFormat);
    m_timer.setSingleShot(true);
    m_timer.setInterval(2000);
    connect(&m_timer, &QTimer::timeout, this, &AiassistantSetting::sync);
}

AiassistantSetting *AiassistantSetting::instance()
{
    static AiassistantSetting set;
    return &set;
}

void AiassistantSetting::setTTSEnable(bool enable)
{
    m_setting->setValue("tts/enable",enable);
    m_timer.start();
}

bool AiassistantSetting::getTTSEnable()
{
    return m_setting->value("tts/enable", true).toBool();
}

void AiassistantSetting::setEnableWindow(bool enable)
{
    m_setting->setValue("tts/enableWindow", enable);
    m_timer.start();
}

bool AiassistantSetting::getEnableWindow()
{
    return m_setting->value("tts/enableWindow", true).toBool();
}

const QSet<QString> &AiassistantSetting::supportedTrans() const
{
    static QSet<QString> ret {"en|cn", "cn|en"};
    return ret;
}

void AiassistantSetting::setTransEnable(bool on)
{
    m_setting->setValue("trans/enable", on);
    m_timer.start();
}

bool AiassistantSetting::getTransEnable()
{
    return m_setting->value("trans/enable", true).toBool();
}

void AiassistantSetting::setTransLanguage(const QString &language)
{
    if (supportedTrans().contains(language)) {
        m_setting->setValue("trans/language", language);
        m_timer.start();
    }
}

QString AiassistantSetting::getTransLanguage()
{
    QString str = m_setting->value("trans/language").toString();
    if (supportedTrans().contains(str))
        return str;
    else
        return *supportedTrans().begin();
}

void AiassistantSetting::setIatEnable(bool on)
{
    m_setting->setValue("iat/enable", on);
    m_timer.start();
}

bool AiassistantSetting::getIatEnable()
{
    return m_setting->value("iat/enable", true).toBool();
}

void AiassistantSetting::sync()
{
    m_setting->sync();
}
