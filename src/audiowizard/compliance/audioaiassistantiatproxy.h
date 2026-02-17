#ifndef AUDIOAIASSISTANTIATPROXY_H
#define AUDIOAIASSISTANTIATPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AudioAiassistant;
class AudioAiassistantIatProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.iat")
public:
    explicit AudioAiassistantIatProxy(AudioAiassistant *parent = nullptr);
    ~AudioAiassistantIatProxy();
    inline QString proxyPath() const {
        return "/aiassistant/iat";
    }
public slots:
    void setIatEnable(bool on);
    bool getIatEnable();
    bool setEos(int eos);
    int getEos();
    bool setBos(int bos);
    int getBos();
    bool setBosWarning(int warningTime);
    int getBosWarning();
    void setIatLanguage(QString language);
    QString getIatLanguage();

private:
    AudioAiassistant *q;
};

UOSAI_END_NAMESPACE

#endif // AUDIOAIASSISTANTIATPROXY_H
