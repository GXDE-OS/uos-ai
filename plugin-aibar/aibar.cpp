
#include "aibar.h"
#include "aibarconfig.h"

#include <pluginfactory.h>
#include "report/aibarpoint.h"
#include "report/eventlogutil.h"

#ifdef HAVE_DDE_SHELL_XDG_ACTIVATION
#include <dde-shell/xdgactivation.h>
#endif

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QUrl>
#include <QtDBus>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logAIBar, "uosai.aibar")

DS_USE_NAMESPACE

namespace uos_ai {

bool AiBar::isTreelandSession() const
{
#ifdef COMPILE_ON_V25
    const auto environment = QProcessEnvironment::systemEnvironment();
    const QString sessionType = environment.value(QStringLiteral("XDG_SESSION_TYPE"));
    const QString waylandDisplay = environment.value(QStringLiteral("WAYLAND_DISPLAY"));

    return sessionType == QLatin1String("wayland")
           || waylandDisplay.contains(QLatin1String("wayland"), Qt::CaseInsensitive);
#else
    return false;
#endif
}

QString AiBar::launchChatPageByDBus(const QString &activationToken) const
{
    QDBusInterface notification("com.deepin.copilot",
                                "/com/deepin/copilot",
                                "com.deepin.copilot",
                                QDBusConnection::sessionBus());

    if (!activationToken.isEmpty()) {
        return notification.call(QDBus::Block, "launchChatPageWithToken", 0, activationToken).errorMessage();
    }

    qCDebug(logAIBar) << "Activation token is empty, attempting to launch chat page without token.";
    return notification.call(QDBus::Block, "launchChatPage").errorMessage();
}

AiBar::AiBar(QObject *parent) : DAppletDock(parent)
{
    ConfigerIns();

    // connect(&drag, &DragMonitor::dragEnter, this, &AiBar::dragActivated);
    // connect(&meetingAssistant, &MeetingAssistant::sigMeetAssistantStatusChanged, this, &AiBar::sigMeetAssistantStatusChanged);
    updateItemList();
}

DockItemInfo AiBar::dockItemInfo()
{
    DockItemInfo info;
    info.name = "uos-ai";
    info.displayName = tr("UOS AI");
    info.itemKey = "uos-ai";
    info.settingKey = "uos-ai";
    info.visible = visible();
    info.dccIcon = "dcc-uos-ai.dci";
    return info;
}

void AiBar::onClickRecommend()
{
    meetingAssistant.onClickRecommend();
}

void AiBar::onClickIcon()
{
#ifdef HAVE_DDE_SHELL_XDG_ACTIVATION
    if (isTreelandSession()) {
        auto xdgActivation = new ds::XdgActivation(this);
        connect(xdgActivation, &ds::XdgActivation::tokenReady, this, [this, xdgActivation](const QString &token) {
            qCDebug(logAIBar) << "Attempting to launch AI assistant";
            qCDebug(logAIBar) << "Found copilot service, attempting to launch chat page";

            const QString error = launchChatPageByDBus(token);
            if (error.isEmpty()) {
                qCDebug(logAIBar) << "Successfully launched chat page with token via DBus";
                xdgActivation->deleteLater();
                return;
            }

            qCWarning(logAIBar) << "Failed to launch chat page with token via DBus:" << error;
            xdgActivation->deleteLater();
        });
        xdgActivation->requestToken();
        return;
    }
#endif

    qCDebug(logAIBar) << "Attempting to launch AI assistant";
    qCDebug(logAIBar) << "Found copilot service, attempting to launch chat page";
    const QString error = launchChatPageByDBus();
    if (error.isEmpty()) {
        qCDebug(logAIBar) << "Successfully launched chat page via DBus";
        return;
    }

    qCWarning(logAIBar) << "Failed to launch chat page via DBus:" << error;
}

MeetingAssistant::MeetAssistantStatus AiBar::getNowMeetAssistantStatus()
{
    return meetingAssistant.getNowMeetAssistantStatus();
}

bool AiBar::isSupportDrop(const QString &url) const
{
    qCDebug(logAIBar) << "Checking if file is supported:" << url;
    QString file = QUrl(url).path();
    static QStringList suffix = {"txt", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "pdf"};

    QFileInfo info (file);
    bool ret = info.isReadable() && suffix.contains(info.suffix(), Qt::CaseInsensitive) && info.size() / 1024 / 1024 <= 100;
    qCDebug(logAIBar) << "File support check result:" << ret << "for file:" << file;
    return ret;
}

void AiBar::handleDrop(const QString &url) const
{
    qCInfo(logAIBar) << "Handling dropped file:" << url;
    QString file = QUrl(url).path();
    uosai.sendFile(file);
}

void AiBar::docAction(int type, const QString &file) const
{
    if (!isSupportDrop(file)) {
        qCWarning(logAIBar) << "Unsupported file type for action:" << file;
        return;
    }

    QString trueFile = file;
    if (file.startsWith("file://")) {
        trueFile = file.mid(7); // 移除前7个字符，即"file://"
    }

    qCDebug(logAIBar) << "Performing document action type:" << type << "on file:" << trueFile;
    switch (type) {
    case Summary:
        uosai.summaryFile(trueFile);
        break;
    case Translation:
        uosai.translateFile(trueFile);
        break;
    case Correction:
        uosai.correctFile(trueFile);
        break;
    case AddToKnowledgeBase:
        uosai.addToKnowledgeBase(trueFile);
        break;
    default:
        qCWarning(logAIBar) << "Unknown document action type:" << type;
        break;
    }
}

void AiBar::onShowDocArea() const
{
    ReportIns()->writeEvent(report::AiBarPoint().assemblingData());
}

void AiBar::updateItemList()
{
    QList<QVariant> itemList;
    AIItem item1(tr("Summarize"),tr("I'll summarize the document for you"),"summary");
    itemList.append(QVariant::fromValue(item1));
    AIItem item2(tr("Translate"),tr("I'll translate the document for you"),"translate");
    itemList.append(QVariant::fromValue(item2));
    AIItem item3(tr("Check for misspelt"),tr("I'll check for misspelt in your document"),"checkForTypos");
    itemList.append(QVariant::fromValue(item3));
    AIItem item4(tr("Add to Knowledge Base"),tr("I'll add the document to the knowledge base"),"knowledgeBase");
    itemList.append(QVariant::fromValue(item4));
    m_itemList = std::move(itemList);
}

QList<QVariant> AiBar::getItemList() const
{
    return m_itemList;
}

bool AiBar::getEnableFileDrag() const
{
    return ConfigerIns()->getEnableFileDrag();
}

D_APPLET_CLASS(AiBar)
}

#include "aibar.moc"
