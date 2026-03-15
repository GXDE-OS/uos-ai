// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ttswidget.h"

#include "modelwrapper/ifly/ttsiflymodel.h"
#include "audio/audioplayerstream.h"
#include "dbus/networkmonitor.h"
#include "utils/util.h"
#include "utils/esystemcontext.h"

#include <DPlatformWindowHandle>

#include <QApplication>
#include <QScreen>
#include <QUuid>

#ifdef COMPILE_ON_V25
#include <ddeshellwayland.h>
#endif

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

#define TTS_TEXT_MAX_LENGTH 2000

TtsWidget::TtsWidget(QWidget *parent)
    : DWidget(parent)
    , m_pushButton(new DIconButton(this))
    , m_closeButton(new DWindowCloseButton(this))
    , m_effectWidget(new DBlurEffectWidget(this))
{
    initUI();

    m_player = new AudioPlayer();
    connect(m_player, &AudioPlayer::playerStreamStopped, this, &TtsWidget::playStop);
    connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged, this, &TtsWidget::networkChanged);
}

TtsWidget::~TtsWidget()
{
    stop();

    delete m_player;
    m_player = nullptr;
}

bool TtsWidget::isWorking()
{
    return m_isworking;
}

bool TtsWidget::startTTS(QString text, bool windowVisible)
{
    if (m_ttsModel || m_isworking) {
        qCWarning(logAudioWizard) << "TTS already in progress";
        return false;
    }

    if (!NetworkMonitor::getInstance().isOnline()) {
        qCCritical(logAudioWizard) << "Network unavailable for TTS";
        Util::playSystemSound_SSE_Error();
        return false;
    }

    qCDebug(logAudioWizard) << "Starting TTS with text length:" << text.length();
    m_isworking = true;
    m_currentID = QUuid::createUuid().toString();

    if (1) {
        //后期增加判断模型的接口
        m_ttsModel = new TtsIflyModel(m_currentID, this);
        m_ttsModel->setModel(TtsModel::ModelType::Ifly);
    }

    connect(m_ttsModel, &TtsModel::error, this, &TtsWidget::ttsServerError, Qt::QueuedConnection);
    connect(m_ttsModel, &TtsModel::appendAudioData, this, &TtsWidget::ttsAudioData, Qt::QueuedConnection);

    if (text.size() > TTS_TEXT_MAX_LENGTH) {
        qCInfo(logAudioWizard) << "Text truncated to max length";
        text = tr("The text you have selected has exceeded the 2000 character limit.");
    }

    m_player->startStream(m_currentID);
    m_ttsModel->sendText(text, true, true);

    qCDebug(logAudioWizard) << "Started TTS with ID:" << m_currentID;
    m_iconTimer.start(1000);
    if (windowVisible) {
        show();
        raise();
    }

    return true;
}

bool TtsWidget::stopTTS()
{
    if (!m_isworking) {
        return false;
    }
    stop();
    return true;
}

void TtsWidget::ttsServerError(int code, const QString &errorString)
{
    qCWarning(logAudioWizard) << "TTS server error - code:" << code << "message:" << errorString;
    stopTTS();
}

void TtsWidget::ttsAudioData(const QString &id, const QByteArray &data, bool isLast)
{
    if (m_currentID != id) {
        qCWarning(logAudioWizard) << "Mismatched audio data ID, expected:" << m_currentID << "received:" << id;
        return;
    }
    qCDebug(logAudioWizard) << "Received TTS audio data - size:" << data.size() << "isLast:" << isLast;
    m_player->appendStreamAudio(id, data, isLast);
}

void TtsWidget::btnIconChange()
{
    if (m_index > 4)
        m_index = 2;
    m_pushButton->setIcon(QIcon(QString(":/assets/icons/tts/play-0%1.svg").arg(m_index)));
    m_index++;
}

void TtsWidget::networkChanged(bool online)
{
    if (!m_isworking)
        return;

    if (!online) {
        qWarning() << tr("Network is offline , stop text to speech");
        Util::playSystemSound_SSE_Error();
        stopTTS();
    }
}

void TtsWidget::playStop(const QString &id)
{
    if (m_currentID != id) {
        qWarning() << "stop unmatched id" << id << "curret id is" << id;
        return;
    }
    qInfo() << "tts end" << id;
    stop();
}

void TtsWidget::initUI()
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(QSize(100, 50));
    installEventFilter(this);

    m_effectWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    m_effectWidget->lower();
    m_effectWidget->setRadius(18);
    m_effectWidget->setFixedSize(QSize(100, 50));

    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);

    DPlatformWindowHandle handle(this);
    handle.setWindowRadius(18);

    m_pushButton->setFlat(true);
    m_pushButton->setFixedSize(QSize(50, 50));
    m_pushButton->move(0, 0);
    m_pushButton->setIconSize(QSize(24, 18));
    m_pushButton->setIcon(QIcon(":/assets/icons/tts/play-01.svg"));

    m_closeButton->setFlat(true);
    m_closeButton->setIconSize(QSize(50, 50));
    m_closeButton->move(50, 0);

    // Get the screen where the mouse cursor is located
    QPoint cursorPos = QCursor::pos();
    QScreen *cursorScreen = nullptr;

    // Find the screen that contains the cursor position
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(cursorPos)) {
            cursorScreen = screen;
            break;
        }
    }

    // Fallback to primary screen if cursor screen not found
    if (!cursorScreen) {
        cursorScreen = QGuiApplication::primaryScreen();
    }

    QRect availableRect = cursorScreen->availableGeometry();
    QPoint pos((availableRect.width() - this->width()) / 2, availableRect.height() - this->height());
    this->move(cursorScreen->geometry().topLeft() + pos - QPoint(0, 10));
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        this->move(pos - QPoint(0, 80));
    }
#endif

    connect(m_closeButton, &DPushButton::clicked, this, &TtsWidget::stopTTS);
    connect(&m_iconTimer, &QTimer::timeout, this, &TtsWidget::btnIconChange);

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        this->createWinId();
        DDEShellWayland::get(windowHandle())->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
        DDEShellWayland::get(windowHandle())->setAcceptKeyboardFocus(false);
        DDEShellWayland::get(windowHandle())->setPosition(this->pos() - QPoint(0, 80));
    }
#endif
}

void TtsWidget::stop()
{
    qCDebug(logAudioWizard) << "Stopping TTS";
    m_player->stopPlayer();
    m_iconTimer.stop();
    if (m_ttsModel) {
        qCDebug(logAudioWizard) << "Canceling TTS model";
        m_ttsModel->cancel();
        delete m_ttsModel;
        m_ttsModel = nullptr;
    }

    m_isworking = false;
    m_pushButton->setIcon(QIcon(":/assets/icons/tts/play-01.svg"));
    hide();
}

bool TtsWidget::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        return true;
    }

    return DWidget::eventFilter(watched, event);
}
