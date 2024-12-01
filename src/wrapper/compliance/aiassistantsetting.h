#ifndef AIASSISTANTSETTING_H
#define AIASSISTANTSETTING_H

#include "uosai_global.h"

#include <QObject>
#include <QTimer>
#include <QSet>

class QSettings;
UOSAI_BEGIN_NAMESPACE

class AiassistantSetting : public QObject
{
    Q_OBJECT
public:
    static AiassistantSetting *instance();
    //tts
    void setTTSEnable(bool enable);
    bool getTTSEnable();
    void setEnableWindow(bool enable);
    bool getEnableWindow();
    //trans
    const QSet<QString> &supportedTrans() const;
    void setTransEnable(bool on);
    bool getTransEnable();
    void setTransLanguage(const QString &language);
    QString getTransLanguage();
    //iat
    void setIatEnable(bool on);
    bool getIatEnable();
signals:

public slots:
protected:
    explicit AiassistantSetting(QObject *parent = nullptr);
    void sync();
private:
    QSettings *m_setting = nullptr;
    QTimer m_timer;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTSETTING_H
