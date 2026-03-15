#include "researchtools.h"
#include "agent/mcpclient.h"
#include "gui/chat/private/eparserdocument.h"
#include "localmodelserver.h"

#include <QLoggingCategory>
#include <QIcon>
#include <QFileInfo>
#include <DFileIconProvider>
#include <QApplication>
#include <QBuffer>
#include <QProcess>
#include <DDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(logAgent)
DWIDGET_USE_NAMESPACE

namespace uos_ai {

static QString getAgentSize()
{
    QProcess process;
    process.setProgram("apt-cache");
    process.setArguments({"show", "uos-ai-agent"});
    process.start();
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return QString();
    }

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.trimmed().startsWith("Size:")) {
            QString sizeStr = line.section(':', 1).trimmed();
            bool ok;
            qint64 size = sizeStr.toLongLong(&ok);
            if (ok) {
                if (size > 1024 * 1024) {
                    return QString("%1M").arg(size / (1024 * 1024));
                } else {
                    return QString("%1K").arg(size / 1024);
                }
            }
        }
    }
    return QString();
}

static void showAgentInstallPrompt()
{
    QString sizeStr = getAgentSize();
    QString bodyText;
    if (!sizeStr.isEmpty()) {
        bodyText = QObject::tr("The uos-ai-agent plugin (approximately %1) must be installed to save documents.").arg(sizeStr);
    } else {
        bodyText = QObject::tr("The uos-ai-agent plugin must be installed to save documents.");
    }

    bool shouldDownload = false;

    auto showDialog = [&]() {
        DDialog dlg(nullptr);
        dlg.setTitle(QObject::tr("Note"));
        dlg.setMessage(bodyText);
        dlg.addButton(QObject::tr("Cancel"), false, DDialog::ButtonNormal);
        dlg.addButton(QObject::tr("Download Now"), true, DDialog::ButtonRecommend);
        if (dlg.exec() == DDialog::Accepted) {
            shouldDownload = true;
        }
    };

    if (QThread::currentThread() == qApp->thread()) {
        showDialog();
    } else {
        QMetaObject::invokeMethod(qApp, showDialog, Qt::BlockingQueuedConnection);
    }

    if (shouldDownload) {
        LocalModelServer::getInstance().openInstallWidget("uos-ai-agent");
    }
}

QString ResearchTools::readDocument(const QString &documentPath)
{
    return EParserDocument::instance()->parserSync(documentPath);
}

bool ResearchTools::md2Word(const QString &md, const QString &wordPath)
{
    McpClient mcpClient;
    if (!mcpClient.init()) {
        qCWarning(logAgent) << "Failed to initialize McpClient for WebScraper.";
        showAgentInstallPrompt();
        return false;
    }

    // Define the agent, tool, and parameters for the MCP server call.
    const QString agentName = "research-agent";
    const QString toolName = "uos-deepresearch.md_to_word";
    QJsonObject params;
    params["text"] = md;
    params["path"] = wordPath;

    qCInfo(logAgent) << "Calling MCP tool 'md_to_word'";

    QPair<int, QString> result = mcpClient.callTool(agentName, toolName, params);

    if (result.first != 0) {
        return false; // Return empty string on failure.
    }

    return true;
}

bool ResearchTools::md2Pdf(const QString &md, const QString &pdfPath)
{
    McpClient mcpClient;
    if (!mcpClient.init()) {
        qCWarning(logAgent) << "Failed to initialize McpClient for WebScraper.";
        showAgentInstallPrompt();
        return false;
    }

    // Define the agent, tool, and parameters for the MCP server call.
    const QString agentName = "research-agent";
    const QString toolName = "uos-deepresearch.md_to_pdf";
    QJsonObject params;
    params["text"] = md;
    params["path"] = pdfPath;

    qCInfo(logAgent) << "Calling MCP tool 'md_to_pdf'";

    QPair<int, QString> result = mcpClient.callTool(agentName, toolName, params);

    if (result.first != 0) {
        return false; // Return empty string on failure.
    }

    return true;
}

bool ResearchTools::checkAgentInstalled()
{
    McpClient mcpClient;
    if (!mcpClient.init()) {
        showAgentInstallPrompt();
        return false;
    }
    return true;
}

QString ResearchTools::getFileIconData(const QString &docPath)
{
    if (docPath.isEmpty())
        return QString();

    QFileInfo docInfo(docPath);
    if (!docInfo.exists())
        return QString();

    QStringList suffixReplace;
    suffixReplace << "docx"
                  << "xlsx"
                  << "pptx";

    if (suffixReplace.contains(docInfo.suffix())) {
        // 文档后缀带x会获取到压缩包图标，需要去掉x: *.docx -> *.doc
        QString replaceXDocPath = docPath;
        replaceXDocPath.remove(replaceXDocPath.size() - 1, 1);
        docInfo.setFile(replaceXDocPath);
    }

    DFileIconProvider docIcon;
    QIcon icon = docIcon.icon(docInfo);

    if (icon.isNull())
        return QString();

    int size = static_cast<int>(qApp->devicePixelRatio() * 16);
    QImage image = icon.pixmap(size, size).toImage();
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return QString::fromLatin1(data.toBase64());
}

void ResearchTools::outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown)
{
    QString title = outlineObj["title"].toString().trimmed();

    if (!title.isEmpty()) {
        QString headerPrefix;
        for (int i = 0; i < level; ++i)
            headerPrefix += "#";
        markdown += QString("%1 %2\n\n").arg(headerPrefix, title);
    }

    if (outlineObj.contains("content") && outlineObj["content"].isArray()) {
        QJsonArray contentArray = outlineObj["content"].toArray();
        for (const QJsonValue &childValue : contentArray)
        {
            outlineJson2Md(childValue.toObject(), level + 1, markdown);
        }
    }
}

} // namespace uos_ai
