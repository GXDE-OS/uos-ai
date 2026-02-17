#ifndef AUDIOAIASSISTANTSETTING_H
#define AUDIOAIASSISTANTSETTING_H

#include "uosai_global.h"

#include <QObject>
#include <QTimer>
#include <QSet>

class QSettings;
UOSAI_BEGIN_NAMESPACE

class AudioAiassistantSetting : public QObject
{
    Q_OBJECT
public:
    static AudioAiassistantSetting *instance();
    //tts
    void setTTSEnable(bool enable);
    bool getTTSEnable();
    void setEnableWindow(bool enable);
    bool getEnableWindow();
    //iat
    void setIatEnable(bool on);
    bool getIatEnable();
    void setIatLanguage(QString language);
    QString getIatLanguage();
    bool setEos(int eos);
    int  getEos();
    bool setBos(int bos);
    int  getBos();
    bool  setBosWarning(int warningTime);
    int  getBosWarning();
    QVariantMap getIatParam();

signals:

public slots:
protected:
    explicit AudioAiassistantSetting(QObject *parent = nullptr);
    void sync();
private:
};

UOSAI_END_NAMESPACE

#endif // AUDIOAIASSISTANTSETTING_H
