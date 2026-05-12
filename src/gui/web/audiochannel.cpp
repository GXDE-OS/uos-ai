#include "audiochannel.h"
#include "audiocontroler.h"
#include "global_define.h"
#include "global_key_define.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QCoreApplication>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

AudioChannel::AudioChannel(QObject *parent)
    : QObject(parent)
{
    // Connect to AudioController signals
    connect(AudioControler::instance(), &AudioControler::levelUpdated,
            this, [this](int level) {
        QJsonObject data;
        data.insert("level", level);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AeLevelUpdated, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });

    connect(AudioControler::instance(), &AudioControler::textReceived,
            this, [this](const QString &text, bool isEnd) {
        QJsonObject data;
        data.insert("text", text);
        data.insert("isEnd", isEnd);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AeTextReceived, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });
    
    connect(AudioControler::instance(), &AudioControler::playTextFinished,
            this, [this](const QString &id) {
        QJsonObject data;
        data.insert("id", id);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AePlayFinished, id, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });
    
    //非硬件报错统一安AudioNetworkError处理
    connect(AudioControler::instance(), &AudioControler::recordError,
            this, [this](int code, const QString &errorString) {
        QJsonObject data;
        data.insert(STR_KEY_ERROR, AudioNetworkError);
        data.insert(STR_KEY_HTTP_ERROR, code);
        data.insert(STR_KEY_ERROR_MESSAGE, errorString);

        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AeRecordError, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });
    
    connect(AudioControler::instance(), &AudioControler::playerError,
            this, [this](int code, const QString &errorString) {
        QJsonObject data;
        data.insert(STR_KEY_ERROR, AudioNetworkError);
        data.insert(STR_KEY_HTTP_ERROR, code);
        data.insert(STR_KEY_ERROR_MESSAGE, errorString);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AePlayerError, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });

    connect(AudioControler::instance(), &AudioControler::playDeviceChanged,
            this, [this](bool valid) {
        QJsonObject data;
        data.insert("valid", valid);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AePlayDeviceChanged, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });
    
    connect(AudioControler::instance(), &AudioControler::recordDeviceChange,
            this, [this](bool valid) {
        QJsonObject data;
        data.insert("valid", valid);
        QJsonDocument doc(data);
        emit audioEvent(AudioEvent::AeRecordDeviceChanged, "", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    });
    
    qCDebug(logAIGUI) << "AudioChannel initialized successfully";
}

AudioChannel::~AudioChannel()
{
}

void AudioChannel::startRecorder(const QString &params)
{

    Q_UNUSED(params);
    
    bool success = AudioControler::instance()->startRecorder();

    if (success) {
        sendSuccess("", {});
    } else {
        sendError("", GErrorType::AudioInputDeviceInvalid, "Failed to start recorder");
    }
}

void AudioChannel::stopRecorder(const QString &params)
{
    Q_UNUSED(params);
    
    bool success = AudioControler::instance()->stopRecorder();
    if (success) {
        sendSuccess("", {});
    } else {
        sendError("", -1, "Failed to stop recorder");
    }
}

void AudioChannel::playTextAudio(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "playTextAudio error: " << err.errorString();
        sendError("", -1, "Invalid JSON parameters");
        return;
    }
    
    auto root = doc.object();
    QString id = root.value("id").toString();
    QString text = root.value("text").toString();
    bool isEnd = root.value("isEnd").toBool(false);
    
    if (id.isEmpty()) {
        qCWarning(logAIGUI) << "playTextAudio error: id is empty";
        sendError("", -1, "Missing required parameter: id");
        return;
    }
    
    if (text.isEmpty()) {
        qCWarning(logAIGUI) << "playTextAudio error: text is empty";
        sendError(id, -1, "Missing required parameter: text");
        return;
    }
    
    bool success = AudioControler::instance()->startAppendPlayText(id, text, isEnd);
    if (success) {
        sendSuccess(id, {});
    } else {
        sendError(id, GErrorType::AudioOutputDeviceInvalid, "Failed to play text audio");
    }
}

void AudioChannel::stopPlayTextAudio(const QString &params)
{
    Q_UNUSED(params);
    
    bool success = AudioControler::instance()->stopPlayTextAudio();
    if (success) {
        sendSuccess("", {});
    } else {
        sendError("", -1, "Failed to stop audio playback");
    }
}

void AudioChannel::playSystemSound(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "playSystemSound error: " << err.errorString();
        sendError("", -1, "Invalid JSON parameters");
        return;
    }
    
    auto root = doc.object();
    int effect = root.value("effect").toInt(0);
    
    AudioControler::AudioSystemEffect audioEffect;
    switch (effect) {
    case 0:
        audioEffect = AudioControler::AudioSystemEffect::Active;
        break;
    case 1:
        audioEffect = AudioControler::AudioSystemEffect::Sleep;
        break;
    default:
        qCWarning(logAIGUI) << "playSystemSound error: invalid effect value" << effect;
        sendError("", -1, "Invalid effect value. Use 0 (Active) or 1 (Sleep)");
        return;
    }
    
    AudioControler::instance()->playSystemSound(audioEffect);
    sendSuccess("", {});
}

void AudioChannel::switchModel(const QString &params)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(params.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(logAIGUI) << "switchModel error: " << err.errorString();
        sendError("", -1, "Invalid JSON parameters");
        return;
    }
    
    auto root = doc.object();
    int model = root.value("model").toInt(0);
    
    AudioControler::AudioModel audioModel;
    switch (model) {
    case 0:
        audioModel = AudioControler::AudioModel::NetWork;
        break;
    case 1:
        audioModel = AudioControler::AudioModel::Local;
        break;
    default:
        qCWarning(logAIGUI) << "switchModel error: invalid model value" << model;
        sendError("", -1, "Invalid model value. Use 0 (Network) or 1 (Local)");
        return;
    }
    
    bool success = AudioControler::instance()->switchModel(audioModel);
    if (success) {
        QVariantMap data;
        data.insert("model", model);
        data.insert("modelName", model == 0 ? "Network" : "Local");
        sendSuccess("", data);
    } else {
        sendError("", -1, "Failed to switch audio model");
    }
}

QString AudioChannel::getDeviceStatus(const QString &params)
{
    Q_UNUSED(params);
    
    QJsonObject data;
    data.insert("inputValid", AudioControler::audioInputDeviceValid());
    data.insert("outputValid", AudioControler::audioOutputDeviceValid());
    
    QJsonDocument doc(data);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void AudioChannel::sendError(const QString &id, int code, const QString &message)
{
    QJsonObject data;
    data.insert("code", code);
    data.insert("message", message);
    QJsonDocument doc(data);
    emit audioEvent(AudioEvent::AeError, id, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    qCWarning(logAIGUI) << "AudioChannel error:" << "id=" << id << "code=" << code << "message=" << message;
}

void AudioChannel::sendSuccess(const QString &id, const QVariantMap &data)
{
    QJsonObject obj;
    for (auto it = data.begin(); it != data.end(); ++it) {
        obj.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    QJsonDocument doc(obj);
    emit audioEvent(AudioEvent::AeSuccess, id, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    qCDebug(logAIGUI) << "AudioChannel success:" << "id=" << id;
}
