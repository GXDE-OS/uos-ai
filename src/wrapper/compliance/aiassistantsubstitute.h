// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AIASSISTANTSUBSTITUTE_H
#define AIASSISTANTSUBSTITUTE_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AiassistantSubstitutePrivate;
class AiassistantSubstitute : public QObject
{
    Q_OBJECT
    friend class AiassistantSubstitutePrivate;
public:
    explicit AiassistantSubstitute(QObject *parent = nullptr);
    ~AiassistantSubstitute();
    bool registerInterface();
signals:

public :
    //tts
    bool isTTSInWorking();
    void stopTTSDirectly();

//mainwindow
    void textToSpeech();
private:
    AiassistantSubstitutePrivate *d = nullptr;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTSUBSTITUTE_H
