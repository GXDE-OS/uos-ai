#ifndef AUDIODBUSINTERFACE_H
#define AUDIODBUSINTERFACE_H

#include <QObject>
#include <QSharedPointer>

#include <com_deepin_daemon_audio.h>
#include <com_deepin_daemon_audio_sink.h>
#include <com_deepin_daemon_audio_source.h>

using com::deepin::daemon::Audio;
using com::deepin::daemon::audio::Sink;
using com::deepin::daemon::audio::Source;

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
