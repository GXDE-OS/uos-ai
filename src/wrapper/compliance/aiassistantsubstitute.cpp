// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aiassistantsubstitute_p.h"
#include "utils/util.h"
#include "utils/compliance/atspidesktop.h"
#include "utils/compliance/qselectionmonitor.h"

#include "dbus/compliance/aiassistantmainwindowproxy.h"
#include "dbus/compliance/aiassistanttransproxy.h"
#include "dbus/compliance/aiassistantiatproxy.h"
#include "dbus/compliance/aiassistantttsproxy.h"

#include <QDBusConnection>
#include <QDebug>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>


#define  EVENT_TIME_INTERVAL 10

UOSAI_USE_NAMESPACE

AiassistantSubstitutePrivate::AiassistantSubstitutePrivate(AiassistantSubstitute *parent) : q(parent)
{

}

AiassistantSubstitutePrivate::~AiassistantSubstitutePrivate()
{
    if (m_tts) {
        delete m_tts;
        m_tts = nullptr;
    }
}

QString AiassistantSubstitutePrivate::getSelectedString()
{
    QString content = "";
    if (Util::isAccessibleEnable()) {
        auto atspi = AtspiDesktop::getInstance();
        auto sel = QSelectionMonitor::getInstance();
        // 在语音记事本中获取选中文本时不走atspi逻辑,永远使用QClipboard::Selection,因为
        // 语音记事本定制选中文本,会主动设置QClipboard::Selection
        int seletedPid = atspi->getSelectedProgramPId();
        QString seletedPidName = Util::getExeNameByPid(seletedPid);

        bool isVoicenote = seletedPidName.contains("voice-note");
        if (isVoicenote) {
            content = sel->getCurSelText();
            qDebug() << "QSelection voice-note :" << content;
        } else {
            if (atspi->isCaptureTextSuccess()) {
                content = atspi->getSelectedContext();
                qDebug() << "ATSPI:" << content;
            } else {
                qInfo() << "fail to get selected text from atspi";
                content =sel->getCurSelText();
                qDebug() << "QSelection:" << content;
            }
        }
    } else {
        qInfo() << "Atspi is disabled.";
        // todo wayland下读取选中文本
/*        if (Util::isWayland()) {
            content = TextToScreenTool::waylandtext();
            TextToScreenTool::setWaylandtext(nullptr);
            qWarning() << "can not get selected text in wayland.";
        } else*/ {
            QClipboard *clipboard = QApplication::clipboard();
            content = clipboard->text(QClipboard::Selection);
        }
    }

    return content;
}

AiassistantSubstitute::AiassistantSubstitute(QObject *parent)
    : QObject(parent)
    , d(new AiassistantSubstitutePrivate(this))
{

}

AiassistantSubstitute::~AiassistantSubstitute()
{
    delete d;
    d = nullptr;
}

bool AiassistantSubstitute::registerInterface()
{
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (!connection.registerService("com.iflytek.aiassistant")){
        qWarning() << "service 'com.iflytek.aiassistant' is already registered by another application";
        return false;
    }

    // init AtspiDesktop
    AiassistantSetting::instance();
    AtspiDesktop::getInstance()->start();
    QSelectionMonitor::getInstance();

    AiassistantMainWindowProxy *mainWin = new AiassistantMainWindowProxy(this);
    connection.registerObject(mainWin->proxyPath(), mainWin, QDBusConnection::ExportAllSlots);

    AiassistantIatProxy* iatdbusServiceProxy = new AiassistantIatProxy(this);
    connection.registerObject(iatdbusServiceProxy->proxyPath(), iatdbusServiceProxy, QDBusConnection::ExportAllSlots);

    AiassistantTtsProxy* ttsdbusServiceProxy = new AiassistantTtsProxy(this);
    connection.registerObject(ttsdbusServiceProxy->proxyPath(), ttsdbusServiceProxy, QDBusConnection::ExportAllSlots);

    AiassistantTransProxy* transdbusServiceProxy = new AiassistantTransProxy(this);
    connection.registerObject(transdbusServiceProxy->proxyPath(), transdbusServiceProxy, QDBusConnection::ExportAllSlots);

    return true;
}

bool AiassistantSubstitute::isTTSInWorking()
{
    bool ret = false;
    if (d->m_tts)
        ret = d->m_tts->isWorking();
    return ret;
}

void AiassistantSubstitute::stopTTSDirectly()
{
    if (d->m_tts)
        d->m_tts->stopTTS();
}

void AiassistantSubstitute::textToSpeech()
{
    if (!AiassistantSetting::instance()->getTTSEnable()) {
        qWarning() << "Tts switch is off";
        return;
    }

    auto currentTime = QDateTime::currentSecsSinceEpoch();
    if ((currentTime - d->m_lastExecutionTime) < EVENT_TIME_INTERVAL) {
        qWarning() << "Shortcut operation is too frequent, please try again later";
        return;
    }

    d->m_lastExecutionTime = currentTime;

    QString text = d->getSelectedString();
    if (text.trimmed().isEmpty()) {
        qInfo() << "no selected text to speech";
        Util::playSystemSound_SSE_Error();
        return;
    }

    if (d->m_ttsContent  == text &&
            d->m_tts && d->m_tts->isWorking()) {
        d->m_tts->stopTTS();
        qInfo() << "tts content is not change and tts is in working ,so stop tts!";
        return;
    }

    if (!d->m_tts) {
        d->m_tts = new TtsCompositeWidget();
    }

    d->m_ttsContent = text;
    d->m_tts->stopTTS();
    d->m_tts->startTTS(text);
}
