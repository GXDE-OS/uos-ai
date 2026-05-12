#ifndef AUDIOCHANNEL_H
#define AUDIOCHANNEL_H

#include <QObject>
#include <QVariantMap>

#include "audiocontroler.h"

namespace uos_ai {

class AudioChannel : public QObject
{
    Q_OBJECT
public:
    explicit AudioChannel(QObject *parent = nullptr);
    ~AudioChannel() override;

signals:
    void audioEvent(int event, const QString &id, const QString &json);

public slots:
    /**
     * @brief startRecorder - Start audio recording
     * @param params - JSON string with parameters (optional)
     */
    void startRecorder(const QString &params = "{}");
    
    /**
     * @brief stopRecorder - Stop audio recording
     * @param params - JSON string with parameters (optional)
     */
    void stopRecorder(const QString &params = "{}");
    
    /**
     * @brief playTextAudio - Play text as audio (TTS)
     * @param params - JSON string with parameters: {"id": "unique_id", "text": "text to play", "isEnd": boolean}
     */
    void playTextAudio(const QString &params);
    
    /**
     * @brief stopPlayTextAudio - Stop text audio playback
     * @param params - JSON string with parameters (optional)
     */
    void stopPlayTextAudio(const QString &params = "{}");
    
    /**
     * @brief playSystemSound - Play a system sound effect
     * @param params - JSON string with parameters: {"effect": 0 or 1} (0=Active, 1=Sleep)
     */
    void playSystemSound(const QString &params);
    
    /**
     * @brief switchModel - Switch between network and local audio model
     * @param params - JSON string with parameters: {"model": 0 or 1} (0=NetWork, 1=Local)
     */
    void switchModel(const QString &params);
    
    /**
     * @brief getDeviceStatus - Get audio device status
     * @param params - JSON string with parameters (optional)
     * @return JSON string with device status
     */
    QString getDeviceStatus(const QString &params = "{}");
    
private:
    /**
     * @brief sendError - Send error event
     * @param id - operation ID
     * @param code - error code
     * @param message - error message
     */
    void sendError(const QString &id, int code, const QString &message);
    
    /**
     * @brief sendSuccess - Send success event
     * @param id - operation ID
     * @param data - additional data
     */
    void sendSuccess(const QString &id, const QVariantMap &data = QVariantMap());
};

} // namespace uos_ai

#endif // AUDIOCHANNEL_H
