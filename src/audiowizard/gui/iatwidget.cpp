#include "iatwidget.h"
#include "../private/eventmonitor.h"
#include "../modelwrapper/ifly/iatiflymodel.h"
#include <dbus/fcitxinputserver.h>
#include <utils/dconfigmanager.h>
#include "utils/esystemcontext.h"
#include "dbus/dbuscontrolcenterrequest.h"
#include "networkmonitor.h"
#include "audioaiassistant.h"

#include <DPushButton>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DPlatformWindowHandle>

#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QHBoxLayout>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QPalette>
#include <QDateTime>

#ifdef COMPILE_ON_V25
#include <ddeshellwayland.h>
#endif

#define WIDGET_WIDTH  268
#define WIDGET_HEIGHT  50

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE
using namespace uos_ai;

int IatWidget::getRecoderVolume() {
#ifdef COMPILE_ON_V25
    QString dbusAudioService = "org.deepin.dde.Audio1";
    QString dbusAudioPath = "/org/deepin/dde/Audio1";
#else
    QString dbusAudioService = "com.deepin.daemon.Audio";
    QString dbusAudioPath = "/com/deepin/daemon/Audio";
#endif
    // 获取麦克风设备
    QDBusInterface interface( dbusAudioService,
                              dbusAudioPath,
                              "org.freedesktop.DBus.Properties");
    QDBusReply<QDBusVariant> reply = interface.call("Get", dbusAudioService, "DefaultSource");
    if (!reply.isValid()) {
        qCritical() << "dbus call "<<dbusAudioService<<" DefaultSource FAILED:" << reply.error().name();
        return -1;
    }

    QString defaultSource = reply.value().variant().value<QDBusObjectPath>().path();
    qInfo() << "dbus call "<<dbusAudioService<<" DefaultSource:" << defaultSource;
    if (defaultSource.isEmpty()) {
        qWarning() << "dbus call "<<dbusAudioService<<" DefaultSource EMPTY";
        return -1;
    }

    // 判断是否静音
    QDBusInterface interface1( dbusAudioService,
                               defaultSource,
                               "org.freedesktop.DBus.Properties");
    reply = interface1.call("Get", dbusAudioService + ".Source", "Name");
    if(!reply.isValid()){
        qWarning() << "dbus call "<<dbusAudioService<<" Name FAILED:" << reply.error().name();
        return -1;
    }

    QString name = reply.value().variant().toString();
    qInfo() << "dbus call "<<dbusAudioService<<" Name:" << name;
    if (name.endsWith("monitor")) {
        qWarning() << "dbus call "<<dbusAudioService<<" Name INVALID";
        return -1;
    }

    reply = interface1.call("Get", dbusAudioService + ".Source", "Mute");
    if(!reply.isValid()){
        qWarning() << "dbus call "<<dbusAudioService<<" Mute FAILED:" << reply.error().name();
        return -1;
    }

    bool isMute = reply.value().variant().toBool();
    if (isMute) {
        qInfo() << "the microphone is muted";
        return 0;
    }

    // 获取麦克风音量
    QDBusInterface interface2( dbusAudioService,
                               defaultSource,
                               "org.freedesktop.DBus.Properties");
    reply = interface2.call("Get", dbusAudioService + ".Source", "Volume");
    if(!reply.isValid()){
        qWarning() << "dbus call "<<dbusAudioService<<" Volume FAILED:" << reply.error().name();
        return -1;
    }

    int volume = int(reply.value().variant().toDouble() * 100);
    qInfo() << "FINALLY, get the volume value:" << volume;
    return volume;
}

IatWidget::IatWidget(QObject *parent) : DWidget(), m_parent(parent)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    this->initUi();
    this->initConnect();

    m_initMs = QDateTime::currentMSecsSinceEpoch();
}

IatWidget::~IatWidget() {

}

void IatWidget::initUi() {
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

    this->setFixedSize(QSize(WIDGET_WIDTH, WIDGET_HEIGHT));
    QPoint pos((availableRect.width() - this->width()) / 2, availableRect.height() - this->height());
    this->move(cursorScreen->geometry().topLeft() + pos - QPoint(0, 10));
    this->installEventFilter(this);
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        this->move(pos - QPoint(0, 80));
    }
#endif

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(247, 247, 247, int(0.6 * 255)));
    this->setPalette(palette);
    this->setAutoFillBackground(true);

    DPlatformWindowHandle handle(this);
    handle.setWindowRadius(18);

    m_effectWidget = new DBlurEffectWidget(this);
    m_effectWidget->setFixedSize(this->size());
    m_effectWidget->setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    m_effectWidget->lower();

    m_animationWidget = new VoiceAnimationWidget(this);
    m_animationWidget->setFixedSize(this->size());
    m_animationWidget->setVisible(false);
    //m_animationWidget->startPaint();
    //m_animationWidget->updateMaxLevel(100);

    m_infoLabel = new DLabel(this);
    m_infoLabel->setText(tr("Speak now"));
    palette = m_infoLabel->palette();
    palette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    m_infoLabel->setPalette(palette);
    m_infoLabel->setVisible(false);

    m_warnLabel = new DLabel(this);
    QColor color = DPaletteHelper::instance()->palette(m_warnLabel).color(DPalette::Normal, DPalette::Highlight);
    m_warnLabel->setText(tr("Low input volume")
                         + QString("<a href=\"javascript:void(0)\" style=\"color: %1; text-decoration: none;\">  %2</a>")
                         .arg(color.name())
                         .arg(tr("Settings")));
    m_warnLabel->setContextMenuPolicy(Qt::NoContextMenu);
    palette = m_warnLabel->palette();
    palette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    m_warnLabel->setPalette(palette);
    m_warnLabel->setVisible(false);
    connect(m_warnLabel, &DLabel::linkActivated, this, &IatWidget::onOpenConfigDialog);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_warnLabel);
    this->setLayout(mainLayout);

    m_closeBt = new DWindowCloseButton(this);
    m_closeBt->setIconSize(QSize(WIDGET_HEIGHT, WIDGET_HEIGHT));
    palette = m_closeBt->palette();
    palette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    m_closeBt->setPalette(palette);
    m_closeBt->move(this->width() - WIDGET_HEIGHT, 0);

    this->onUpdateSystemTheme();

#ifdef COMPILE_ON_V25
    if (ESystemContext::isTreeland()) {
        this->createWinId();
        DDEShellWayland::get(windowHandle())->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
        DDEShellWayland::get(windowHandle())->setAcceptKeyboardFocus(false);
        DDEShellWayland::get(windowHandle())->setPosition(this->pos() - QPoint(0, 80));
    }
#endif
}

void IatWidget::initConnect() {
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &IatWidget::onUpdateSystemTheme);

    connect(m_closeBt, &DWindowCloseButton::clicked, this, &IatWidget::onBtClicked);

    //connect(EventMonitor::instance(), &EventMonitor::sendMouseClickedEventsignal, this, &IatWidget::onMouseClicked);
    //connect(EventMonitor::instance(), &EventMonitor::sendKeyPressEventsignal, this, &IatWidget::onKeyPressed);

    // 依赖fcitx：当前可写状态
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusIn, this, &IatWidget::onFocusIn);
    connect(&FcitxInputServer::getInstance(), &FcitxInputServer::signalFocusOut, this, &IatWidget::onFocusOut);

    // 避免多个实例同时出现
    connect(dynamic_cast<AudioAiassistant *>(m_parent), &AudioAiassistant::sigIatTriggered, this, &IatWidget::close);

    // network
    //connect(&NetworkMonitor::getInstance(), &NetworkMonitor::stateChanged, this, &IatWidget::onNetworkStateChanged);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [&] {
        // 没说话的超时提醒
        m_timer->stop();
        m_animationWidget->stopPaint();
        m_animationWidget->setVisible(false);
        m_infoLabel->setVisible(true);
    });
}

bool IatWidget::startIat()
{
    qCDebug(logAudioWizard) << "Starting IAT process";
    m_volume = IatWidget::getRecoderVolume();
    if (m_volume < 0) {
        qCWarning(logAudioWizard) << "No valid audio input device found";
        this->close();
        return false;
    }

    this->show();
    if (m_volume < 10) {
        qCInfo(logAudioWizard) << "Low microphone volume detected";
        m_warnLabel->setVisible(true);
        return true;
    }

    if (!NetworkMonitor::getInstance().isOnline()) {
        qCWarning(logAudioWizard) << "Network unavailable for IAT";
        m_isOnline = false;
        m_infoLabel->setText(tr("Network unavailable"));
        m_infoLabel->setVisible(true);
        return true;
    }

    qCDebug(logAudioWizard) << "Creating audio recorder and model";
    m_animationWidget->setVisible(true);
    m_animationWidget->startPaint();

    m_recorder = new AudioRecorder();
    m_model = new IatIflyModel(this);
    connect(m_recorder, &AudioRecorder::recordStarted, this, [&] {
        m_model->sendDataStart();
    });
    connect(m_recorder, &AudioRecorder::audioRecorded, this, &IatWidget::onAudioData);
    connect(m_recorder, &AudioRecorder::recordStoped, this, [&] {
        m_model->sendDataEnd();
    });
    connect(m_recorder, &AudioRecorder::recordError, this, [&] {
        qWarning() << "record ERROR";
        this->close();
    });
    connect(m_recorder, &AudioRecorder::levelUpdated, this, [&] (int level) {
        m_animationWidget->updateMaxLevel(level * 2);
    });
    connect(m_model, &IatModel::textReceived, this, &IatWidget::onTextReceived);
    connect(m_model, &IatModel::error, this, [&](int code, QString msg) {
        qWarning() << QString("code(%1) msg(%2)").arg(code).arg(msg);
        this->onTextReceived(msg, true);
    });
    if (!m_recorder->start()) {
        qCCritical(logAudioWizard) << "Failed to start audio recorder";
        this->close();
        return false;
    }

    // 要求延时时间可配置
    int delay = DConfigManager::instance()->value(AUDIOWIZARD_GROUP, AUDIOWIZARD_IAT_NOSPEAK_DELAY_MS, 5000).toInt();
    if (delay <= 0) {
        delay = 1;
    }
    qCDebug(logAudioWizard) << "Starting no-speech timeout timer with delay:" << delay << "ms";
    m_timer->start(delay);
    return true;
}

bool IatWidget::stopIat() {
    this->close();
    return true;
}

void IatWidget::showEvent(QShowEvent *event) {
    DWidget::showEvent(event);
}

void IatWidget::closeEvent(QCloseEvent *event) {
    qCDebug(logAudioWizard) << "Closing IAT widget";
#ifdef COMPILE_ON_V23
    FcitxInputServer::getInstance().setPreEditOn(false);
#endif

    m_timer->stop();
    m_animationWidget->stopPaint();
    if (m_recorder) {
        qCDebug(logAudioWizard) << "Stopping audio recorder";
        m_recorder->stop();
        delete m_recorder;
        m_recorder = nullptr;
    }
    if (m_model) {
        qCDebug(logAudioWizard) << "Canceling IAT model";
        m_model->cancel();
        delete m_model;
        m_model = nullptr;
    }
    this->deleteLater();
    DWidget::closeEvent(event);
}

void IatWidget::resizeEvent(QResizeEvent *event) {
    m_effectWidget->resize(event->size());
    DWidget::resizeEvent(event);
}

void IatWidget::paintEvent(QPaintEvent *event) {
    DWidget::paintEvent(event);
}

bool IatWidget::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        return true;
    }

    return DWidget::eventFilter(watched, event);
}

void IatWidget::onUpdateSystemTheme() {
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        QPalette palette = this->palette();
        palette.setColor(QPalette::Window, QColor(247, 247, 247, int(0.6 * 255)));
        this->setPalette(palette);

        palette = m_infoLabel->palette();
        palette.setColor(QPalette::WindowText, QColor(0, 0, 0));
        m_infoLabel->setPalette(palette);
        m_warnLabel->setPalette(palette);

        palette = m_closeBt->palette();
        palette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
        m_closeBt->setPalette(palette);
    } else {
        QPalette palette = this->palette();
        palette.setColor(QPalette::Window, QColor(32, 32, 32, int(0.5 * 255)));
        this->setPalette(palette);

        palette = m_infoLabel->palette();
        palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
        m_infoLabel->setPalette(palette);
        m_warnLabel->setPalette(palette);

        palette = m_closeBt->palette();
        palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
        m_closeBt->setPalette(palette);
    }
}

void IatWidget::onBtClicked() {
    if (sender() == m_closeBt) {
        this->close();
        return;
    }
}

void IatWidget::onAudioData(QByteArray data) {
    //qDebug() << "onAudioData:" << data.size();
    if (!m_model) {
        return;
    }

    m_model->sendData(data);
}

void IatWidget::onOpenConfigDialog() {
#if defined(COMPILE_ON_V25)
    DbusControlCenterRequest dbus;
    dbus.showPage("sound");
#elif defined(COMPILE_ON_V23)
    QProcess::startDetached("dbus-send --print-reply --dest=org.deepin.dde.ControlCenter1 /org/deepin/dde/ControlCenter1 org.deepin.dde.ControlCenter1.ShowPage string:\"sound\"");
#else
    QProcess::startDetached("dbus-send --print-reply --dest=com.deepin.dde.ControlCenter /com/deepin/dde/ControlCenter com.deepin.dde.ControlCenter.ShowPage string:\"sound\" string:\"Microphone\"");
#endif

    this->close();
}

void IatWidget::onMouseClicked(int type, int x, int y) {
    double ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
    qDebug() << QString("ratio(%1) point(%2, %3)").arg(ratio).arg(int(x / ratio)).arg(int(y / ratio));
    if (this->geometry().contains(QPoint(int(x / ratio), int(y / ratio)))) {
        return;
    }

    qDebug() << "QUIT, mouse clicked";
    this->close();
}

void IatWidget::onKeyPressed(int type) {
    qDebug() << "QUIT, key pressed";
    this->close();
}

void IatWidget::onFocusIn() {
    // 控件show的瞬间，焦点会有波动
    if (QDateTime::currentMSecsSinceEpoch() - m_initMs < 1000) {
        return;
    }

    qDebug() << "QUIT, focus changed";
    this->close();
}

void IatWidget::onFocusOut() {
    if (QDateTime::currentMSecsSinceEpoch() - m_initMs < 1000) {
        return;
    }

    qDebug() << "QUIT, focus changed";
    this->close();
}

void IatWidget::onTextReceived(QString text, bool isEnd) {
    if (!text.isEmpty()) {
        m_timer->start(); // restart
        if (m_infoLabel->isVisible()) {
            m_infoLabel->setVisible(false);
            m_animationWidget->setVisible(true);
            m_animationWidget->startPaint();
        }
    }

#ifdef COMPILE_ON_V23
    FcitxInputServer::getInstance().commitToPreEdit(text);
#else
    if (!text.startsWith(m_text)) {
        for (int i = 0; i < m_text.length(); i++) {
            FcitxInputServer::getInstance().deleteChar();
        }
        m_text = text;
        FcitxInputServer::getInstance().commitString(m_text);
    } else {
        FcitxInputServer::getInstance().commitString(text.mid(m_text.length()));
        m_text = text;
    }
#endif

    if (isEnd && this->isVisible()) {
        qCInfo(logAudioWizard) << "IAT process completed with text:" << text;
        QTimer::singleShot(500, this, [&] {
            this->close();
        });
    }
}

#if 0
void IatWidget::onNetworkStateChanged(bool isOnline)
{
    if (isOnline && !m_isOnline) {
        this->close();
        (new IatWidget(m_parent))->startIat();
    }
}
#endif
