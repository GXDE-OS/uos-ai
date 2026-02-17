// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AUDIOAIASSISTANT_H
#define AUDIOAIASSISTANT_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class BaseClipboard;
class AudioAiassistantPrivate;
class AudioAiassistant : public QObject
{
    Q_OBJECT
    friend class AudioAiassistantPrivate;
public:
    explicit AudioAiassistant(QObject *parent = nullptr);
    ~AudioAiassistant();
    bool registerInterface();

public :
    //tts
    bool isTTSInWorking();
    void stopTTSDirectly();

    void textToSpeech();
    void speechToText();
    QString startAsr(const QVariantMap &param);
    void stopAsr();
signals:
    void onNotify(const QString &msg);
    void sigIatTriggered();
private:
    AudioAiassistantPrivate *d = nullptr;
    BaseClipboard *m_selectclip = nullptr;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANT_H
