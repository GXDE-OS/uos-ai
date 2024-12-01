// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ttscompositewidget.h"

#include "audio/server/ttssocketserver.h"
#include "audio/audioplayerstream.h"
#include "dbus/networkmonitor.h"
#include "utils/util.h"

#include <DPlatformWindowHandle>

#include <QDesktopWidget>
#include <QApplication>
#include <QScreen>
#include <QUuid>

DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

#define TTS_TEXT_MAX_LENGTH 5000

TtsCompositeWidget::TtsCompositeWidget(QWidget *parent)
    : DWidget(parent)
    , m_pushButton(new DIconButton(this))
    , m_effectWidget(new DBlurEffectWidget(this))
{
    initUI();

    m_player = new AudioPlayer();
    connect(m_player, &AudioPlayer::playerStreamStopped, this, &TtsCompositeWidget::playStop);
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged, this, &TtsCompositeWidget::networkChanged);
}

TtsCompositeWidget::~TtsCompositeWidget()
{
    stop();

    delete m_player;
    m_player = nullptr;
}

bool TtsCompositeWidget::isWorking()
{
    return m_isworking;
}

bool TtsCompositeWidget::startTTS(QString text, bool ui)
{
    if (m_ttsServer || m_isworking) {
        qWarning() << "there is a tts server in working.";
        return false;
    }

    if (!NetworkMonitor::getInstance().isOnline()) {
        qCritical() << "Network is not Online , can not startTTS";
        Util::playSystemSound_SSE_Error();
        return false;
    }

    m_isworking = true;

    m_currentID = QUuid::createUuid().toString();
    m_ttsServer = new TtsSocketServer(m_currentID, AccountProxy::xfInlineAccount(), this);

    connect(m_ttsServer, &TtsServer::error, this, &TtsCompositeWidget::ttsServerError, Qt::QueuedConnection);
    connect(m_ttsServer, &TtsServer::appendAudioData, this, &TtsCompositeWidget::ttsAudioData, Qt::QueuedConnection);
    m_ttsServer->openServer();

    if (text.size() > TTS_TEXT_MAX_LENGTH)
        text = tr("The text you have selected has exceeded the 5000 character limit.");

    m_player->startStream(m_currentID);
    m_ttsServer->sendText(text, true, true);

    m_iconTimer.start(1000);

    if (ui) {
        move(QCursor::pos());
        show();
        raise();
    }

    return true;
}

bool TtsCompositeWidget::stopTTS()
{
    stop();
    return true;
}

void TtsCompositeWidget::ttsServerError(int code, const QString &errorString)
{
    qWarning() << "tts error:" << code << errorString;
    stopTTS();
}

void TtsCompositeWidget::ttsAudioData(const QString &id, const QByteArray &data, bool isLast)
{
    if (m_currentID != id) {
        qWarning() << "get unmatched id" << id << "curret id is" << id;
        return;
    }
    qDebug() << "recv tts adudio" << id << data.size() << isLast;
    m_player->appendStreamAudio(id, data, isLast);
}

void TtsCompositeWidget::btnIconChange()
{
    if (m_index > 4)
        m_index = 2;
    m_pushButton->setIcon(QIcon(QString(":/assets/icons/ttscomposite/reading%1.svg").arg(m_index)));
    m_index++;
}

void TtsCompositeWidget::networkChanged(bool online)
{
    if (!m_isworking)
        return;

    if (!online) {
        qWarning() << "Network is offline , can not continue TTS and stopTTS";
        Util::playSystemSound_SSE_Error();
        stopTTS();
    }
}

void TtsCompositeWidget::playStop(const QString &id)
{
    if (m_currentID != id) {
        qWarning() << "stop unmatched id" << id << "curret id is" << id;
        return;
    }
    qDebug() << "tts end" << id;
    stop();
}

void TtsCompositeWidget::initUI()
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(QSize(48, 48));

    m_effectWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    m_effectWidget->lower();
    m_effectWidget->setRadius(24);
    m_effectWidget->setFixedSize(QSize(48, 48));

    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);

    DPlatformWindowHandle handle(this);
    handle.setWindowRadius(24);

    m_pushButton->setFlat(true);
    m_pushButton->setFixedSize(this->size());
    m_pushButton->move(0, 0);
    m_pushButton->setIconSize(QSize(30, 30));
    m_pushButton->setIcon(QIcon(":/assets/icons/ttscomposite/reading1.svg"));

    QScreen *desktopWidget = QGuiApplication::primaryScreen();
    QRect availableRect = desktopWidget->availableGeometry();
    this->move(availableRect.right() * 3 / 5, availableRect.height() * 4 / 5);

    connect(m_pushButton, &DPushButton::clicked, this, &TtsCompositeWidget::stopTTS);
    connect(&m_iconTimer, &QTimer::timeout, this, &TtsCompositeWidget::btnIconChange);
}

void TtsCompositeWidget::stop()
{
    m_player->stopPlayer();
    m_iconTimer.stop();
    if (m_ttsServer) {
        m_ttsServer->cancel();
        delete m_ttsServer;
        m_ttsServer = nullptr;
    }

    m_isworking = false;
    m_pushButton->setIcon(QIcon(":/assets/icons/ttscomposite/reading1.svg"));
    hide();
}
