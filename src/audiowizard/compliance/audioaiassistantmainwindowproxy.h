#ifndef AUDIOAIASSISTANTMAINWINDOWPROXY_H
#define AUDIOAIASSISTANTMAINWINDOWPROXY_H

#include <QObject>

namespace uos_ai {
class AudioAiassistant;
class AudioAiassistantMainWindowProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.mainWindow")
public:
    inline QString proxyPath() const{
        return "/aiassistant/deepinmain";
    }
public:
    AudioAiassistantMainWindowProxy(AudioAiassistant* parent);
    ~AudioAiassistantMainWindowProxy();

public slots:
    //朗读接口
    void TextToSpeech();
    //听写接口
    void SpeechToText();
    //转写接口
    QString startAsr(const QVariantMap &param);
    void stopAsr();
    //翻译接口
    void TextToTranslate();

private slots:
    // 网络状态变化处理
    void onNetworkStateChanged(bool online);

signals:
    void onNotify(const QString &msg);
private:
    AudioAiassistant *q;
};

}

#endif // AUDIOAIASSISTANTMAINWINDOWPROXY_H
