#include "audiodbusinterface.h"

#include <QDebug>
#include <QAudioDeviceInfo>

AudioDbusInterface::AudioDbusInterface(QObject *parent)
    : QObject(parent)
    , m_audioInter(new Audio("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio", QDBusConnection::sessionBus(), this))
{
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
    if (QAudioDeviceInfo::defaultInputDevice().deviceName() != m_defaultSource->name())
        return false;


    const AudioPort &activePort = m_defaultSource->activePort();
    if (activePort.name.isEmpty())
        return false;

    if (!m_audioInter->IsPortEnabled(m_defaultSource->card(), activePort.name))
        return false;

    return activePort.availability == 2 || activePort.availability == 0;
}

bool AudioDbusInterface::isDefaultOutputDeviceValid()
{
    if (QAudioDeviceInfo::defaultOutputDevice().deviceName() != m_defaultSink->name())
        return false;

    const AudioPort &activePort = m_defaultSink->activePort();
    if (activePort.name.isEmpty())
        return false;

    if (!m_audioInter->IsPortEnabled(m_defaultSink->card(), activePort.name))
        return false;

    return activePort.availability == 2 || activePort.availability == 0;
}

void AudioDbusInterface::setDefaultSource(const QDBusObjectPath &defaultSource)
{
    m_defaultSource.reset(new Source("com.deepin.daemon.Audio", m_audioInter->defaultSource().path(), QDBusConnection::sessionBus(), this));
    connect(m_defaultSource.data(), &Source::ActivePortChanged, this, &AudioDbusInterface::defaultInputChanged, Qt::QueuedConnection);
    QTimer::singleShot(5, this, SIGNAL(defaultInputChanged()));
}

void AudioDbusInterface::setDefaultSink(const QDBusObjectPath &defaultSink)
{
    m_defaultSink.reset(new Sink("com.deepin.daemon.Audio", m_audioInter->defaultSink().path(), QDBusConnection::sessionBus(), this));
    connect(m_defaultSink.data(), &Sink::ActivePortChanged, this, &AudioDbusInterface::defaultOutputChanged, Qt::QueuedConnection);
    QTimer::singleShot(5, this, SIGNAL(defaultOutputChanged()));
}

void AudioDbusInterface::onPortEnabledChanged(uint id, const QString &name, bool enable)
{
    if (id == m_defaultSink->card() && m_defaultSink->activePort().name == name) {
        emit defaultOutputChanged();
    } else  if (id == m_defaultSource->card() && m_defaultSource->activePort().name == name) {
        emit defaultInputChanged();
    }
}
