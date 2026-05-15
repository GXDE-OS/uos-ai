#include "researchtools.h"
#include "iconstore.h"
#include "agent/mcpclient.h"
#include "localmodelserver.h"
#include "services/fileservice/fileservice.h"

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

Q_DECLARE_LOGGING_CATEGORY(logResearch)
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
        bodyText = QCoreApplication::translate("ResearchTools", "The uos-ai-agent plugin (approximately %1) must be installed to save documents.").arg(sizeStr);
    } else {
        bodyText = QCoreApplication::translate("ResearchTools", "The uos-ai-agent plugin must be installed to save documents.");
    }

    bool shouldDownload = false;

    auto showDialog = [&]() {
        DDialog dlg(nullptr);
        dlg.setTitle(QCoreApplication::translate("ResearchTools","Note"));
        dlg.setMessage(bodyText);
        dlg.addButton(QCoreApplication::translate("ResearchTools","Cancel"), false, DDialog::ButtonNormal);
        dlg.addButton(QCoreApplication::translate("ResearchTools","Download Now"), true, DDialog::ButtonRecommend);
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
    QString err;
    QString content = FileService::parseFileSync(documentPath, &err);
    if (!err.isEmpty())
        qCWarning(logResearch) << "readDocument error: " << err;

    return content;
}

bool ResearchTools::md2Word(const QString &md, const QString &wordPath)
{
    McpClient mcpClient;
    if (!mcpClient.init()) {
        qCWarning(logResearch) << "Failed to initialize McpClient for WebScraper.";
        showAgentInstallPrompt();
        return false;
    }

    // Define the agent, tool, and parameters for the MCP server call.
    const QString agentName = "research-agent";
    const QString toolName = "uos-deepresearch.md_to_word";
    QJsonObject params;
    params["text"] = md;
    params["path"] = wordPath;

    qCInfo(logResearch) << "Calling MCP tool 'md_to_word'";

    QPair<int, QString> result = mcpClient.callTool(agentName, toolName, params);

    if (result.first != 0) {
        qCWarning(logResearch) << "ResearchTools::md2Word failed, error:" << result.first << result.second;
        return false;
    }

    return true;
}

bool ResearchTools::md2Pdf(const QString &md, const QString &pdfPath)
{
    McpClient mcpClient;
    if (!mcpClient.init()) {
        qCWarning(logResearch) << "Failed to initialize McpClient for WebScraper.";
        showAgentInstallPrompt();
        return false;
    }

    // Define the agent, tool, and parameters for the MCP server call.
    const QString agentName = "research-agent";
    const QString toolName = "uos-deepresearch.md_to_pdf";
    QJsonObject params;
    params["text"] = md;
    params["path"] = pdfPath;

    qCInfo(logResearch) << "Calling MCP tool 'md_to_pdf'";

    QPair<int, QString> result = mcpClient.callTool(agentName, toolName, params);

    if (result.first != 0) {
        qCWarning(logResearch) << "ResearchTools::md2Pdf failed, error:" << result.first << result.second;
        return false;
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

QString ResearchTools::getFileIconKey(const QString &docPath)
{
    if (docPath.isEmpty())
        return QString();

    QFileInfo docInfo(docPath);
    if (!docInfo.exists())
        return QString();

    // Generate icon key by extension (deduplicates across files with same type)
    QString key = IconStore::extensionKey(docPath);
    if (key.isEmpty())
        return QString();

    // Skip if already cached
    if (IconStore::instance()->exists(key))
        return key;

    // Map docx/xlsx/pptx to doc/xls/ppt for correct system icon
    QStringList suffixReplace;
    suffixReplace << "docx" << "xlsx" << "pptx";

    if (suffixReplace.contains(docInfo.suffix())) {
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

    return IconStore::instance()->saveFromImage(key, image);
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
