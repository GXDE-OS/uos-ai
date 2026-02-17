#ifndef AUDIODBUSINTERFACE_H
#define AUDIODBUSINTERFACE_H

#include <QObject>
#include <QSharedPointer>

#include "dbus/3rdparty/com_deepin_daemon_audio.h"
#include "dbus/3rdparty/com_deepin_daemon_audio_sink.h"
#include "dbus/3rdparty/com_deepin_daemon_audio_source.h"

using uos_ai::Audio;
using uos_ai::audio::Sink;
using uos_ai::audio::Source;

class AudioDbusInterface : public QObject
{
    Q_OBJECT
public:
    explicit AudioDbusInterface(QObject *parent = nullptr);
    static AudioDbusInterface *instance();

    bool isDefaultInputDeviceValid();
    bool isDefaultOutputDeviceValid();

signals:
    void defaultInputChanged();
    void defaultOutputChanged();

private slots:
    void setDefaultSource(const QDBusObjectPath &defaultSource);
    void setDefaultSink(const QDBusObjectPath &defaultSink);

    void onPortEnabledChanged(uint id, const QString &name, bool enable);

private:
    Audio *m_audioInter;
    QSharedPointer<Sink> m_defaultSink;
    QSharedPointer<Source> m_defaultSource;
};

#endif // AUDIODBUSINTERFACE_H
