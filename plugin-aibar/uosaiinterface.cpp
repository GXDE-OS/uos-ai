#include "uosaiinterface.h"
#include <QLoggingCategory>
#include <QtDBus>
#include <QDBusMessage>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

using namespace uos_ai;

UosAiInterface::UosAiInterface(QObject *parent)
    : QObject{parent}
{
    qDBusRegisterMetaType<QMap<QString, QString>>();
}

void UosAiInterface::sendFile(const QString &file) const
{
    qCDebug(logAIBar) << "Sending file to AI:" << file;
    inputPrompt("", file);
}

void UosAiInterface::summaryFile(const QString &file) const
{
    qCDebug(logAIBar) << "Requesting summary for file:" << file;
    inputPrompt(tr("Summarize the content of this document"), file);
}

void UosAiInterface::translateFile(const QString &file) const
{
    qCDebug(logAIBar) << "Requesting translation for file:" << file;
    inputPrompt(tr("Translate the document in its entirety"), file);
}

void UosAiInterface::correctFile(const QString &file) const
{
    qCDebug(logAIBar) << "Requesting spell check for file:" << file;
    inputPrompt(tr("Check for misspelt in this document"), file);
}

void UosAiInterface::addToKnowledgeBase(const QString &file) const
{
    qCDebug(logAIBar) << "Adding file to knowledge base:" << file;
    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.copilot", "/org/deepin/copilot/chat",
                                                      "org.deepin.copilot.chat", "sendToKnowledgeBase");
    QVariantList params;
    params.append(QStringList{file});
    msg.setArguments(params);

    auto con = QDBusConnection::sessionBus();
    con.asyncCall(msg);
}

void UosAiInterface::inputPrompt(const QString &question, const QString &file) const
{
    qCDebug(logAIBar) << "Sending input prompt - Question:" << question << "File:" << file;
    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.copilot", "/org/deepin/copilot/chat",
                                                      "org.deepin.copilot.chat", "inputPrompt");
    QVariantList params;
    QMap<QString, QString> ext;
    ext.insert("file", file);
    ext.insert("defaultPrompt", question);
    params.append("");
    params.append(QVariant::fromValue(ext));
    msg.setArguments(params);

    auto con = QDBusConnection::sessionBus();
    con.asyncCall(msg);
}
