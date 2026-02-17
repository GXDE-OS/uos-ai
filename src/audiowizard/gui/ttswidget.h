// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TTSWIDGET_H
#define TTSWIDGET_H

#include "uosai_global.h"

#include <QTimer>

#include <DWidget>
#include <DPushButton>
#include <DBlurEffectWidget>
#include <DIconButton>
#include <DWindowCloseButton>

#include <QMutex>


class AudioPlayer;
namespace uos_ai {
class TtsModel;
class TtsWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit TtsWidget(QWidget *parent = nullptr);
    ~TtsWidget();
    bool isWorking();
    bool startTTS(QString text, bool windowVisible);
signals:

public slots:
    bool stopTTS();
private slots:
    void ttsServerError(int code, const QString &errorString);
    void ttsAudioData(const QString &id, const QByteArray &data, bool isLast);
    void btnIconChange();
    void networkChanged(bool online);
    void playStop(const QString &id);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUI();
    void stop();
private:
    bool m_isworking = false;
    DTK_WIDGET_NAMESPACE::DIconButton *m_pushButton = nullptr;
    DTK_WIDGET_NAMESPACE::DWindowCloseButton *m_closeButton = nullptr;
    DTK_WIDGET_NAMESPACE::DBlurEffectWidget *m_effectWidget = nullptr;
    QTimer m_iconTimer;
    int m_index = 2;
    TtsModel *m_ttsModel = nullptr;
    AudioPlayer *m_player = nullptr;
    QString m_currentID;
};
}

#endif // TTSWIDGET_H
