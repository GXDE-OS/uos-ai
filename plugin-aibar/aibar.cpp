
#include "aibar.h"
#include "aibarconfig.h"

#include <pluginfactory.h>
#include "report/aibarpoint.h"
#include "report/eventlogutil.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QUrl>
#include <QtDBus>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logAIBar, "uosai.aibar")

DS_USE_NAMESPACE

namespace uos_ai {

AiBar::AiBar(QObject *parent) : DApplet(parent)
{
    ConfigerIns();

    connect(&drag, &DragMonitor::dragEnter, this, &AiBar::dragActivated);
    connect(&meetingAssistant, &MeetingAssistant::sigMeetAssistantStatusChanged, this, &AiBar::sigMeetAssistantStatusChanged);
    updateItemList();
}

bool AiBar::visible() const
{
    return m_visible;
}

void AiBar::setVisible(bool visible)
{
    m_visible = visible;
    emit visibleChanged();
}

void AiBar::onClickRecommend()
{
    meetingAssistant.onClickRecommend();
}

void AiBar::onClickIcon()
{
    qCDebug(logAIBar) << "Attempting to launch AI assistant";
    QDBusConnection connection = QDBusConnection::sessionBus();
    bool isServiceRegistered = connection.interface()->isServiceRegistered("com.deepin.copilot");
    if (isServiceRegistered) {
        qCDebug(logAIBar) << "Found copilot service, attempting to launch chat page";
        QDBusInterface notification("com.deepin.copilot", "/com/deepin/copilot", "com.deepin.copilot", QDBusConnection::sessionBus());
        QString error = notification.call(QDBus::Block, "launchChatPage").errorMessage();
        if (error.isEmpty()) {
            qCDebug(logAIBar) << "Successfully launched chat page via DBus";
            return;
        }
        qCWarning(logAIBar) << "Failed to launch chat page via DBus:" << error;
    }
    qCDebug(logAIBar) << "Starting uos-ai-assistant process directly";
    QStringList arguments;
    arguments << "--chat";
    QProcess::startDetached("uos-ai-assistant", arguments);
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