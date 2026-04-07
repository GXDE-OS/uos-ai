#include "audiodbusinterface.h"

#include <QLoggingCategory>
#ifdef COMPILE_ON_QT6
#include <QAudioDevice>
#include <QMediaDevices>
#else
#include <QAudioDeviceInfo>
#endif

Q_DECLARE_LOGGING_CATEGORY(logDBus)

AudioDbusInterface::AudioDbusInterface(QObject *parent)
    : QObject(parent)
{

    // 修复麦克风不可用的问题
    m_audioInter = new Audio("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio", QDBusConnection::sessionBus(), this);

    m_audioInter->setSync(true);
    connect(m_audioInter, &Audio::DefaultSinkChanged, this, &AudioDbusInterface::setDefaultSink, Qt::QueuedConnection);
    connect(m_audioInter, &Audio::DefaultSourceChanged, this, &AudioDbusInterface::setDefaultSource, Qt::QueuedConnection);
    connect(m_audioInter, &Audio::PortEnabledChanged, this, &AudioDbusInterface::onPortEnabledChanged, Qt::QueuedConnection);

    setDefaultSink(m_audioInter->defaultSink());
    setDefaultSource(m_audioInter->defaultSource());
}

AudioDbusInterface *AudioDbusInterface::instance()
{
    static AudioDbusInterface interface;
    return &interface;
}

bool AudioDbusInterface::isDefaultInputDeviceValid()
{
#ifdef COMPILE_ON_QT6
    //QT6用description判断音频设备,如果QMediaDevices的description()为"Echo-Cancel Source"，说明开启了噪音抑制模式，跳过音频设备判断
    if (QMediaDevices::defaultAudioInput().description() != "Echo-Cancel Source" && QMediaDevices::defaultAudioInput().description() != m_defaultSource->description()) {
        qCDebug(logDBus) << "Default input device mismatch";
        return false;
    }
#else
    if (QAudioDeviceInfo::defaultInputDevice().deviceName() != m_defaultSource->name()) {
        qCWarning(logDBus) << "Default input device mismatch";
        return false;
    }
#endif

    const AudioPort &activePort = m_defaultSource->activePort();
    if (activePort.name.isEmpty()) {
        qCWarning(logDBus) << "Active port name is empty";
        return false;
    }

    if (!m_audioInter->IsPortEnabled(m_defaultSource->card(), activePort.name)) {
        qCWarning(logDBus) << "Port not enabled:" << activePort.name;
        return false;
    }

    bool isValid = (activePort.availability == 2 || activePort.availability == 0);
    qCInfo(logDBus) << "Input device validity:" << isValid;
    return isValid;
}

bool AudioDbusInterface::isDefaultOutputDeviceValid()
{
#ifdef COMPILE_ON_QT6
    //QT6用description判断音频设备
    if (QMediaDevices::defaultAudioOutput().description() != m_defaultSink->description()) {
        qCDebug(logDBus) << "Default output device mismatch";
        return false;
    }
#else
    if (QAudioDeviceInfo::defaultOutputDevice().deviceName() != m_defaultSink->name()) {
        qCWarning(logDBus) << "Default output device mismatch";
        return false;
    }
#endif

    const AudioPort &activePort = m_defaultSink->activePort();
    if (activePort.name.isEmpty()) {
        qCWarning(logDBus) << "Active port name is empty";
        return false;
    }

    if (!m_audioInter->IsPortEnabled(m_defaultSink->card(), activePort.name)) {
        qCWarning(logDBus) << "Port not enabled:" << activePort.name;
        return false;
    }

    bool isValid = (activePort.availability == 2 || activePort.availability == 0);
    qCInfo(logDBus) << "Output device validity:" << isValid;
    return isValid;
}

void AudioDbusInterface::setDefaultSource(const QDBusObjectPath &defaultSource)
{
#ifdef COMPILE_ON_V20
    m_defaultSource.reset(new Source("com.deepin.daemon.Audio", m_audioInter->defaultSource().path(), QDBusConnection::sessionBus(), this));
#else
    m_defaultSource.reset(new Source("org.deepin.dde.Audio1", m_audioInter->defaultSource().path(), QDBusConnection::sessionBus(), this));
#endif
    connect(m_defaultSource.data(), &Source::ActivePortChanged, this, &AudioDbusInterface::defaultInputChanged, Qt::QueuedConnection);
    QTimer::singleShot(5, this, SIGNAL(defaultInputChanged()));
    qCDebug(logDBus) << "Default source changed:" << defaultSource.path();
}

void AudioDbusInterface::setDefaultSink(const QDBusObjectPath &defaultSink)
{
#ifdef COMPILE_ON_V20
    m_defaultSink.reset(new Sink("com.deepin.daemon.Audio", m_audioInter->defaultSink().path(), QDBusConnection::sessionBus(), this));
#else
    m_defaultSink.reset(new Sink("org.deepin.dde.Audio1", m_audioInter->defaultSink().path(), QDBusConnection::sessionBus(), this));
#endif
    connect(m_defaultSink.data(), &Sink::ActivePortChanged, this, &AudioDbusInterface::defaultOutputChanged, Qt::QueuedConnection);
    QTimer::singleShot(5, this, SIGNAL(defaultOutputChanged()));
    qCDebug(logDBus) << "Default sink changed:" << defaultSink.path();
}

void AudioDbusInterface::onPortEnabledChanged(uint id, const QString &name, bool enable)
{
    qCDebug(logDBus) << "Port enabled changed - id:" << id << "name:" << name << "enable:" << enable;
    if (id == m_defaultSink->card() && m_defaultSink->activePort().name == name) {
        emit defaultOutputChanged();
    } else if (id == m_defaultSource->card() && m_defaultSource->activePort().name == name) {
        emit defaultInputChanged();
    }
}
