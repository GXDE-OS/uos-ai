#include "conversationchannel.h"
#include "conversation/conversationmanager.h"
#include "conversation/messagenode.h"
#include "agent/research/workspacestore.h"
#include "agent/research/article.h"
#include "agent/research/research_key_define.h"
#include "global_key_define.h"
#include "private/pdfprintpreviewdialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDebug>
#include <QApplication>
#include <QWebEnginePage>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QFileInfo>
#include <QPageLayout>
#include <QPageSize>
#include <QFile>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;

ConversationChannel::ConversationChannel(QObject *parent)
    : QObject(parent)
{
    // 连接 ConversationManager 的 indexChanged 信号
    connect(ConvMgr(), &ConversationManager::indexChanged, this, &ConversationChannel::indexChanged);
    // 连接 ConversationManager 的 changeToConversation 信号
    connect(ConvMgr(), &ConversationManager::changeToConversation, this, &ConversationChannel::changeToConversation);
}

ConversationChannel::~ConversationChannel()
{
}

QJsonObject ConversationChannel::getConversation(const QString &id)
{    
    ConversationRecordPtr record = ConvMgr()->getConversation(id);
    if (!record) {
        qWarning(logAIGUI) << "getConversation: conversation not found:" << id;
        return QJsonObject();
    }
    return record->toJson();
}

void ConversationChannel::deleteConversation(const QStringList &ids)
{
    ConvMgr()->deleteConversation(ids);
}

void ConversationChannel::releaseConversation(const QStringList &ids)
{
    ConvMgr()->releaseConversation(ids);
}

void ConversationChannel::clearAllConversations()
{
    ConvMgr()->clearAllConversations();
}

void ConversationChannel::setConversationRender(const QString &conversationId, const QString &msgId, const QString &renderMsgJson)
{
    ConversationRecordPtr record = ConvMgr()->getConversation(conversationId);
    if (record) {
        MessageNodePtr msgNode = record->messageAt(msgId);
        if (!msgNode) {
            qWarning(logAIGUI) << "message not found:" << msgId;
            return;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(renderMsgJson.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning(logAIGUI) << "setConversationRender error: " << err.errorString();
            return;
        }

        QJsonArray renderArray = doc.array();
        QString introduction;

        for (const QJsonValue &value : renderArray) {
            QJsonObject renderObj = value.toObject();

            RenderMessage renderMsg;

            // Parse type
            QString typeStr = renderObj.value(STR_KEY_TYPE).toString();
            ContentType type = GlobalUtil::contentTypeFromString(typeStr);
            renderMsg.type = type;
            QJsonObject dataObj = renderObj.value(STR_KEY_DATA).toObject();

            // For structured render items, save the entire data object
            // For simple types, save only the content string
            if (type == CntReasoning || type == CntAgentStep || type == CntTool || type == CntOutline || type == CntDocCard || type == CntCommandCard || type == CntError) {
                QVariantHash data = dataObj.toVariantHash();

                // Resolve version=-1 to the actual committed version at storage time,
                // so each DocCard reference points to the exact snapshot for this message.
                if (type == CntDocCard) {
                    int ver = data.value(STR_RESEARCH_VERSION, -1).toInt();
                    if (ver < 0) {
                        QString articleId = data.value(STR_KEY_ID).toString();
                        if (!articleId.isEmpty()) {
                            WritingWorkspace ws = WorkspaceStore::instance()->findByConversationId(conversationId);
                            const Article *art = ws.article(articleId);
                            if (art)
                                data[STR_RESEARCH_VERSION] = art->currentVersionNumber();
                        }
                    }
                }

                renderMsg.data = data;

                // Save to introduction
                if (type == CntAgentStep && dataObj.contains(STR_KEY_TITLE)) {
                    introduction += dataObj[STR_KEY_TITLE].toString();
                } else if (type == CntOutline && dataObj.contains("title")) {
                    introduction += dataObj["title"].toString();
                }
            } else {
                // For simple types, save only the content string
                renderMsg.data = dataObj.value(STR_KEY_CONTENT).toString();
                if (type == CntText) {
                    introduction += dataObj.value(STR_KEY_CONTENT).toString();
                }
            }

            msgNode->appendRender(renderMsg);

            //将对应的question的modelName和modelId复制给answer
            MessageNodePtr preMsgNode = record->messageAt(msgNode->getPrevious());
            if (preMsgNode) {
                msgNode->setModelName(preMsgNode->getModelName());
                msgNode->setModelId(preMsgNode->getModelId());
            } else {
                qWarning(logAIGUI) << "question not found:" << msgNode->getPrevious();
                return;
            }
        }

        if (record->introduction().isEmpty())
            record->setIntroduction(introduction);

    } else {
        qWarning(logAIGUI) << conversationId <<"conversation is not exist";
        return;
    }
}

void ConversationChannel::updateWorkspaceOutline(const QString &conversationId, const QString &outlineJson)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(outlineJson.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning(logAIGUI) << "updateWorkspaceOutline parse error:" << err.errorString();
        return;
    }

    QJsonObject frontendOutline = doc.object();
    if (frontendOutline.isEmpty()) {
        qWarning(logAIGUI) << "updateWorkspaceOutline: empty outline for conversation:" << conversationId;
        return;
    }

    // 前端大纲格式：{ "title": "...", "paragraphs": [...] }
    // WorkspaceStore 格式：{ "title": "...", "content": [...] }
    // 两者仅顶层键名不同（paragraphs → content），子章节结构完全一致
    QJsonObject workspaceOutline;
    workspaceOutline[STR_KEY_TITLE] = frontendOutline.value(STR_KEY_TITLE);
    workspaceOutline[STR_KEY_CONTENT] = frontendOutline.value(STR_KEY_PARAGRAPHS);

    WritingWorkspace ws = WorkspaceStore::instance()->findByConversationId(conversationId);
    if (ws.conversationId().isEmpty()) {
        qWarning(logAIGUI) << "updateWorkspaceOutline: workspace not found for conversation:" << conversationId;
        return;
    }

    Article *art = ws.activeArticle();
    if (!art) {
        qWarning(logAIGUI) << "updateWorkspaceOutline: no active article in workspace:" << conversationId;
        return;
    }

    art->setOutline(workspaceOutline);
    WorkspaceStore::instance()->saveArticle(ws.conversationId(), *art);
    qDebug(logAIGUI) << "updateWorkspaceOutline: saved outline for conversation:" << conversationId;
}

QString ConversationChannel::getWorkspaceOutline(const QString &conversationId, const QString &articleId)
{
    WritingWorkspace ws = WorkspaceStore::instance()->findByConversationId(conversationId);
    if (ws.conversationId().isEmpty()) {
        qWarning(logAIGUI) << "getWorkspaceOutline: workspace not found:" << conversationId;
        return QString();
    }

    const Article *art = ws.article(articleId);
    if (!art || !art->hasOutline()) {
        return QString();
    }

    QJsonObject outline = art->outline();
    QJsonObject frontendOutline;
    frontendOutline[STR_KEY_TITLE] = outline.value(STR_KEY_TITLE);
    frontendOutline[STR_KEY_PARAGRAPHS] = outline.value(STR_KEY_CONTENT);

    return QString::fromUtf8(QJsonDocument(frontendOutline).toJson(QJsonDocument::Compact));
}

QString ConversationChannel::getWorkspaceArticle(const QString &conversationId, const QString &articleId, int version)
{
    WritingWorkspace ws = WorkspaceStore::instance()->findByConversationId(conversationId);
    if (ws.conversationId().isEmpty()) {
        qWarning(logAIGUI) << "getWorkspaceArticle: workspace not found:" << conversationId;
        return QString();
    }

    const Article *art = ws.article(articleId);
    if (!art) {
        art = ws.activeArticle();
    }
    if (!art) {
        qWarning(logAIGUI) << "getWorkspaceArticle: article not found:" << articleId;
        return QString();
    }

    if (version >= 0) {
        const ArticleVersion *ver = art->version(version);
        if (!ver)
            return QString();

        QJsonObject result;
        result[STR_KEY_ID]          = art->id();
        result[STR_KEY_TITLE]       = art->title();
        result[STR_KEY_CONTENT]     = ver->content;
        result[STR_KEY_REFERENCES]  = ws.articleOrderedReferences(art->id());
        result[STR_RESEARCH_VERSION] = ver->version;
        result[STR_KEY_CREATED_AT]  = ver->createdAt.toString(Qt::ISODate);
        result[STR_KEY_UPDATED_AT]  = art->updatedAt().toString(Qt::ISODate);
        return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
    }

    QJsonObject result;
    result[STR_KEY_ID]          = art->id();
    result[STR_KEY_TITLE]       = art->title();
    result[STR_KEY_CONTENT]     = art->currentContent();
    result[STR_KEY_REFERENCES]  = ws.articleOrderedReferences(art->id());
    result[STR_RESEARCH_VERSION] = art->currentVersionNumber();
    result[STR_KEY_UPDATED_AT]  = art->updatedAt().toString(Qt::ISODate);
    result[STR_KEY_CREATED_AT]  = art->createdAt().toString(Qt::ISODate);
    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

bool ConversationChannel::updateWorkspaceArticle(const QString &conversationId, const QString &articleId, const QString &content)
{
    WritingWorkspace ws = WorkspaceStore::instance()->findByConversationId(conversationId);
    if (ws.conversationId().isEmpty()) {
        qWarning(logAIGUI) << "updateWorkspaceArticle: workspace not found:" << conversationId;
        return false;
    }

    Article *art = ws.article(articleId);
    if (!art) {
        art = ws.activeArticle();
    }
    if (!art) {
        qWarning(logAIGUI) << "updateWorkspaceArticle: article not found:" << articleId;
        return false;
    }

    art->updateContent(content);
    bool ok = WorkspaceStore::instance()->saveArticle(ws.conversationId(), *art);
    qDebug(logAIGUI) << "updateWorkspaceArticle: saved article" << art->id() << "ok:" << ok;
    return ok;
}

bool ConversationChannel::saveConversation(const QString &id)
{
    return ConvMgr()->saveConversation(id);
}

QString ConversationChannel::getConversationIndexes()
{
    auto vector = ConvMgr()->conversationIndexes();

    QJsonArray root;
    for (const ConversationIndexItem &indexItem : vector) {
        root.append(indexItem.toJson());
    }

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool ConversationChannel::switchMessageNext(const QString &conversationId, const QString &target, const QString &next)
{
    ConversationRecordPtr record = ConvMgr()->getConversation(conversationId);
    if (!record) {
        qWarning(logAIGUI) << "switchMessageNext: conversation not found:" << conversationId;
        return false;
    }

    bool result = record->switchNext(target, next);
    if (!result) {
        qWarning(logAIGUI) << "switchMessageNext: failed to switch next for target:" << target << "next:" << next;
    }

    return result;
}

bool ConversationChannel::saveWorkspaceArticleToFile(const QString &conversationId, const QString &articleId, const QString &format)
{
    return WorkspaceStore::instance()->exportArticleToFile(conversationId, articleId, format);
}

void ConversationChannel::printHTML(const QString &html, const QString &title)
{
    if (m_isPrinting) {
        qCWarning(logAIGUI) << "Print request ignored because another print task is in progress";
        return;
    }

    if (html.isEmpty()) {
        qCWarning(logAIGUI) << "Print document html is empty";
        return;
    }

    QString outputDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (outputDir.isEmpty()) {
        outputDir = QStringLiteral("/tmp");
    }
    QDir(outputDir).mkpath(".");

    QString safeTitle = title.isEmpty() ? QStringLiteral("document") : title;
    safeTitle.replace(QRegularExpression("[\\/:*?\"<>|\\s]+"), "_");
    const QString docName = title.isEmpty() ? safeTitle : title;

    const QString pdfPath = QDir(outputDir).filePath(
        QStringLiteral("uos-ai-print-%1_%2.pdf")
            .arg(safeTitle, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmss"))));

    m_isPrinting = true;

    QWebEnginePage *page = new QWebEnginePage(this);
    page->setBackgroundColor(Qt::white);

    // 超时保护：防止 loadFinished/pdfPrintingFinished 不触发导致 m_isPrinting 卡死
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [this, page, pdfPath, timeoutTimer]() {
        qCWarning(logAIGUI) << "Print task timed out after 30s, force cleanup";
        m_isPrinting = false;
        QFile::remove(pdfPath);
        page->deleteLater();
        timeoutTimer->deleteLater();
    });
    timeoutTimer->start(30000);

    connect(page, &QWebEnginePage::pdfPrintingFinished, this,
            [this, page, pdfPath, docName, timeoutTimer](const QString &path, bool ok) {
                const auto finishTask = [this, page, pdfPath, timeoutTimer]() {
                    timeoutTimer->stop();
                    timeoutTimer->deleteLater();
                    m_isPrinting = false;
                    QFile::remove(pdfPath);
                    page->deleteLater();
                };

                if (!ok) {
                    qCWarning(logAIGUI) << "Failed to generate PDF:" << path;
                    finishTask();
                    return;
                }

                QFileInfo pdfInfo(path);
                if (!pdfInfo.exists() || pdfInfo.size() <= 0) {
                    qCWarning(logAIGUI) << "PDF file missing or empty:" << path;
                    finishTask();
                    return;
                }

                QWidget *parentWidget = qApp->activeWindow();
                PDFPrintPreviewDialog previewDialog(pdfInfo.absoluteFilePath(), docName, parentWidget);
                if (!previewDialog.isValid()) {
                    qCWarning(logAIGUI) << "Failed to load PDF into preview dialog:" << pdfInfo.absoluteFilePath();
                    finishTask();
                    return;
                }

                previewDialog.exec();
                finishTask();
            });

    connect(page, &QWebEnginePage::loadFinished, this,
            [this, page, pdfPath, timeoutTimer](bool ok) {
                if (!ok) {
                    qCWarning(logAIGUI) << "Failed to load HTML for printing";
                    timeoutTimer->stop();
                    timeoutTimer->deleteLater();
                    m_isPrinting = false;
                    page->deleteLater();
                    return;
                }

                QPageLayout layout(QPageSize(QPageSize::A4),
                                   QPageLayout::Portrait,
                                   QMarginsF(20, 20, 20, 20));
                page->printToPdf(pdfPath, layout);
            });

    page->setHtml(html, QUrl("qrc://"));
}
