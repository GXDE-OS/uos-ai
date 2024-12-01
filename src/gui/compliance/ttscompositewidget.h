// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TTSCOMPOSITEWIDGET_H
#define TTSCOMPOSITEWIDGET_H

#include "uosai_global.h"

#include <QTimer>

#include <DWidget>
#include <DPushButton>
#include <DBlurEffectWidget>
#include <DIconButton>

#include <QMutex>

class TtsSocketServer;
class AudioPlayer;
UOSAI_BEGIN_NAMESPACE

class TtsCompositeWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit TtsCompositeWidget(QWidget *parent = nullptr);
    ~TtsCompositeWidget();
    bool isWorking();
    bool startTTS(QString text, bool ui = true);
signals:

public slots:
    bool stopTTS();
private slots:
    void ttsServerError(int code, const QString &errorString);
    void ttsAudioData(const QString &id, const QByteArray &data, bool isLast);
    void btnIconChange();
    void networkChanged(bool online);
    void playStop(const QString &id);
private:
    void initUI();
    void stop();
private:
    bool m_isworking = false;
    DTK_WIDGET_NAMESPACE::DIconButton *m_pushButton = nullptr;
    DTK_WIDGET_NAMESPACE::DBlurEffectWidget *m_effectWidget = nullptr;
    QTimer m_iconTimer;
    int m_index = 2;
    TtsSocketServer *m_ttsServer = nullptr;
    AudioPlayer *m_player = nullptr;
    QString m_currentID;
};

UOSAI_END_NAMESPACE

#endif // TTSCOMPOSITEWIDGET_H
