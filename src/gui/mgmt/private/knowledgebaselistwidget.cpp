#include "knowledgebaselistwidget.h"
//#include "modellistitem.h"
//#include "addmodeldialog.h"
#include "themedlable.h"
#include "knowledgebaselistitem.h"
#include "embeddingserver.h"
#include "localmodelserver.h"
#include "knowledgebasemanager.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DFileDialog>
#include <DDialog>
#include <DArrowRectangle>
#include <DFloatingWidget>
#include <report/knowledgefilenumberpoint.h>
#include <report/knowledgefunctionpoint.h>
#include <report/eventlogutil.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QProcess>
#include <QScreen>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static const qint64 KnowledgeBaseSize = 1024 * 1024 * 1024;

using namespace uos_ai;

KnowledgeBaseListWidget::KnowledgeBaseListWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
    onThemeTypeChanged();

    m_supportedSuffix << "*.txt"
                      << "*.doc"
                      << "*.docx"
                      << "*.xls"
                      << "*.xlsx"
                      << "*.ppt"
                      << "*.pptx"
                      << "*.pdf";
    m_lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();

    initKnowBaseList();
}

void KnowledgeBaseListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_pDeleteButton = new DCommandLinkButton(tr("Delete"), this);
    DFontSizeManager::instance()->bind(m_pDeleteButton, DFontSizeManager::T8, QFont::Normal);
    m_pDeleteButton->setProperty("editMode", false);
    m_pDeleteButton->hide();
    m_pAddButton = new DCommandLinkButton(tr("Add"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    m_pDeleteButton->setEnabled(false);
    m_pDeleteButton->setToolTip(tr("Please install the embedding model plugins first"));
    m_pAddButton->setEnabled(false);
    m_pAddButton->setToolTip(tr("Please install the embedding model plugins first"));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    m_pWidgetLabel = new ThemedLable(tr("Knowledge Base Management"));
    m_pWidgetLabel->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_pWidgetLabel, DFontSizeManager::T5, QFont::Bold);
    m_pWidgetLabel->setElideMode(Qt::ElideRight);


    m_sizeLabel = new DLabel(this);
    m_sizeLabel->setText(QString("%1M/1024M").arg(m_allFileSize));
    DFontSizeManager::instance()->bind(m_sizeLabel, DFontSizeManager::T8, QFont::Medium);
    m_sizeLabel->setForegroundRole(DPalette::TextTips);

    m_tipIconLabel = new DLabel(this);
    m_tipIconLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_tooltips").pixmap(16, 16));
//    m_tipIconLabel->installEventFilter(this);
    m_tipIconLabel->setToolTip("<p style=\'font-size: 12px\'>"+tr("This feature requires high hardware resources, and the reference benchmark configuration is: CPU Intel 11th generation i7 or above; "
                                  "Memory of 16GB or more; Having a NVIDIA graphics card and a 10 series or higher is the best option. "
                                  "If the configuration is too low, there may be issues such as lagging and inaccurate answers.") + "</p>");

    topLayout->addWidget(m_pWidgetLabel);
    topLayout->addSpacing(5);
    topLayout->addWidget(m_sizeLabel, 0, Qt::AlignCenter);
    topLayout->addSpacing(5);
    topLayout->addWidget(m_tipIconLabel, 0, Qt::AlignCenter);
    topLayout->addStretch();
    topLayout->addWidget(m_pDeleteButton);
    topLayout->addWidget(m_pAddButton);

    DLabel *descLabel = new DLabel(tr("Here, the knowledge base of the Personal Knowledge Assistant can be defined, "
                                      "and the Personal Knowledge Assistant will answer questions based on the files added below."));
    descLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T8, QFont::Normal);
    descLabel->setForegroundRole(DPalette::TextTips);

    layout->addLayout(topLayout);
    layout->addWidget(descLabel);
    layout->addWidget(noKnowledgeBaseWidget());
    layout->addWidget(hasKnowledgeBaseWidget());
}

void KnowledgeBaseListWidget::initConnect()
{
    connect(m_pDeleteButton, &DCommandLinkButton::clicked, this, &KnowledgeBaseListWidget::onEditButtonClicked);
    connect(m_pAddButton, &DCommandLinkButton::clicked, this, &KnowledgeBaseListWidget::onAddKnowledgeBase);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &KnowledgeBaseListWidget::onThemeTypeChanged);
    connect(&KnowledgeBaseManager::getInstance(), &KnowledgeBaseManager::filesProcessing, this, &KnowledgeBaseListWidget::onFilesProcessing);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged, this, &KnowledgeBaseListWidget::onAddToServerStatusChanged);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted, this, &KnowledgeBaseListWidget::onIndexDeleted);
    connect(&LocalModelServer::getInstance(), &LocalModelServer::modelPluginsStatusChanged, this, &KnowledgeBaseListWidget::onEmbeddingPluginsStatusChanged);

}

void KnowledgeBaseListWidget::onThemeTypeChanged()
{
    qCDebug(logAIGUI) << "Theme type changed for KnowledgeBaseListWidget.";
    DPalette pl = m_pHasKnowledgeBaseWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pHasKnowledgeBaseWidget->setPalette(pl);

    DPalette pl1 = m_pNoKnowledgeBaseWidget->palette();
    pl1.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pNoKnowledgeBaseWidget->setPalette(pl1);

    m_tipIconLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_tooltips").pixmap(16, 16));
}

void KnowledgeBaseListWidget::onEditButtonClicked()
{
    qCInfo(logAIGUI) << "Edit button clicked. EditMode:" << m_pDeleteButton->property("editMode").toBool();
    bool editMode = m_pDeleteButton->property("editMode").toBool();
    m_pDeleteButton->setProperty("editMode", !editMode);
    editMode = m_pDeleteButton->property("editMode").toBool();

    auto items = m_pHasKnowledgeBaseWidget->findChildren<KnowledgeBaseItem *>();
    for (auto item : items) {
        item->setEditMode(editMode);
    }

    editMode ? m_pDeleteButton->setText(tr("Done")) : m_pDeleteButton->setText(tr("Delete"));
    m_pAddButton->setEnabled(!editMode);

    if(!editMode)
        EmbeddingServer::getInstance().saveAllIndex();
}

DWidget *KnowledgeBaseListWidget::noKnowledgeBaseWidget()
{
    DWidget *widget = new DWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 0, 10, 0);

    layout->addWidget(new DLabel(tr("None"), widget));
    layout->addStretch();

    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->addWidget(widget);

    m_pNoKnowledgeBaseWidget = new DBackgroundGroup(bgLayout, this);
    m_pNoKnowledgeBaseWidget->setContentsMargins(0, 0, 0, 0);
    m_pNoKnowledgeBaseWidget->setFixedHeight(36);

    return m_pNoKnowledgeBaseWidget;
}

DWidget *KnowledgeBaseListWidget::hasKnowledgeBaseWidget()
{
    DWidget *widget = new DWidget(this);
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_pHasKnowledgeBaseWidget = new DBackgroundGroup(layout, this);
    m_pHasKnowledgeBaseWidget->setContentsMargins(0, 0, 0, 0);
    m_pHasKnowledgeBaseWidget->setItemSpacing(1);
    return m_pHasKnowledgeBaseWidget;
}

void KnowledgeBaseListWidget::removeKnowledgeBase(const QString &name)
{
    qCInfo(logAIGUI) << "Removing knowledge base. Name:" << name;
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + name);
    if(!item)
        return;

    QStringList fileList;
    fileList << item->filePath();

    KnowledgeBaseManager::getInstance().removeFromKnowledgeBase(fileList);
}

bool KnowledgeBaseListWidget::onAppendKnowledgeBase(const QString &filePath, int status)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
    if (item) {
        qCWarning(logAIGUI) << "File already exists in knowledge base. FileName:" << fileName;
        DDialog dialog(tr("File already exist") ,
                       tr("The %1 file already exists and cannot be added again. "
                          "Please modify the file name or delete the existing file before adding it again").arg(fileName), this);
        dialog.setFixedWidth(380);
        dialog.setIcon(QIcon(":assets/images/warning.svg"));
        dialog.addButton(QObject::tr("OK", "button"));
        auto labelList = dialog.findChildren<QLabel *>();
        for (auto messageLabel : labelList) {
            if ("MessageLabel" == messageLabel->objectName()) {
                messageLabel->setFixedWidth(dialog.width() - 20);
            }
        }
        dialog.exec();
        return false;
    }

    createKnowBaseItem(filePath, status);

    return true;
}

void KnowledgeBaseListWidget::onAddKnowledgeBase()
{
    ReportIns()->writeEvent(report::KnowledgeFunctionPoint("Click Upload").assemblingData());
    DFileDialog fileDlg(this);
    fileDlg.setDirectory(m_lastImportPath);
    QString selfilter = tr("All files") + (" (%1)");
    selfilter = selfilter.arg(KnowledgeBaseManager::getInstance().supportedFormats().join(" "));
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(DFileDialog::ExistingFiles);
    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    fileDlg.setObjectName("fileDialogAdd");
    if (DFileDialog::Accepted == fileDlg.exec()) {
        m_lastImportPath = fileDlg.directory().path();
        addKnowledgeBaseFile(fileDlg.selectedFiles());
    }
}

void KnowledgeBaseListWidget::addKnowledgeBaseFile(const QStringList &fileList)
{
    auto result = KnowledgeBaseManager::getInstance().addExistingFiles(fileList);

    if (result.result == KnowledgeBaseManager::AddResult::Success)
        refreshKnowBaseList();
}

void KnowledgeBaseListWidget::onAddToServerStatusChanged(const QStringList &files, int status)
{
    qCDebug(logAIGUI) << "AddToServerStatusChanged. Files:" << files << ", Status:" << status;
    if (files.isEmpty()) {
        // getDoc()新接口statusChanged信号不带参数，直接刷新列表
        refreshKnowBaseList();
    } else {
        // 兼容旧接口
        refreshKnowBaseListOld(files, status);
    }

//    if (status == 1) {
//        TODO 信号弃用，后续清理
//        emit sigGenPersonalFAQ();
//    }
}

void KnowledgeBaseListWidget::onIndexDeleted(const QStringList &files)
{
    qCInfo(logAIGUI) << "Index deleted. Files:" << files;
    foreach(QString file, files) {
        QFileInfo fileInfo(file);
        KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileInfo.fileName());
        if (!item)
            continue;

        updateFileSize(-item->fileSize());
        m_pHasKnowledgeBaseWidget->layout()->removeWidget(item);
        item->setParent(nullptr);
        delete item;

        adjustWidgetSize();
    }
    ReportIns()->writeEvent(report::KnowledgeFileNumberPoint(KnowledgeBaseManager::getInstance().getKnowledgeBaseFiles().size()).assemblingData());
}

void KnowledgeBaseListWidget::onFilesProcessing(const QStringList &files)
{
    for (const QString &file : files)
        onAppendKnowledgeBase(file, KnowledgeBaseProcessStatus::Processing);
}

void KnowledgeBaseListWidget::onEmbeddingPluginsStatusChanged(bool isExist)
{
    qCDebug(logAIGUI) << "Embedding plugins status changed. Exist:" << isExist;
    if (!isExist) {
        resetEditButton();
        m_pDeleteButton->setEnabled(false);
        m_pDeleteButton->setToolTip(tr("Please install the embedding model plugins first"));
        m_pAddButton->setEnabled(false);
        m_pAddButton->setToolTip(tr("Please install the embedding model plugins first"));
    }
    else {
        m_pDeleteButton->setEnabled(true);
        m_pDeleteButton->setToolTip("");
        m_pAddButton->setEnabled(true);
        m_pAddButton->setToolTip("");
    }
}

void KnowledgeBaseListWidget::onRefresh()
{
    refreshKnowBaseList();
}

void KnowledgeBaseListWidget::adjustWidgetSize()
{
    if (m_pHasKnowledgeBaseWidget->layout()->count() <= 0) {
        m_pNoKnowledgeBaseWidget->show();
        m_pHasKnowledgeBaseWidget->hide();
        m_pDeleteButton->hide();
        m_pDeleteButton->setProperty("editMode", false);
        if (checkEmbeddingPluginsStatus())
            m_pAddButton->setEnabled(true);
    } else {
        m_pNoKnowledgeBaseWidget->hide();
        m_pHasKnowledgeBaseWidget->show();
        m_pDeleteButton->show();
    }
    adjustSize();
}

void KnowledgeBaseListWidget::createKnowBaseItem(const QString &filePath, int status)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QFile file(filePath);
    qint64 bytes = 0;
    if (file.exists()) {
        QFileInfo fileInfo(file);
        bytes = fileInfo.size();
    }

    KnowledgeBaseProcessStatus knowStatus = KnowledgeBaseProcessStatus::Processing;
    switch (status) {
    case 0:
        knowStatus = KnowledgeBaseProcessStatus::Processing;
        break;
    case -1:  // 兼容getDocFiles V1.0接口
    case 1:
        knowStatus = KnowledgeBaseProcessStatus::Succeed;
        break;
    case 2:
        knowStatus = KnowledgeBaseProcessStatus::FileError;
        break;
    case 3:
        knowStatus = KnowledgeBaseProcessStatus::ProcessingError;
        break;
    }

    KnowledgeBaseItem *newItem = new KnowledgeBaseItem(fileName, filePath, this);
    newItem->setStatus(knowStatus);
    newItem->setFileSize(bytes);
    updateFileSize(bytes);

    connect(newItem, &KnowledgeBaseItem::signalDeleteItem, this, &KnowledgeBaseListWidget::removeKnowledgeBase, Qt::QueuedConnection);
    m_pHasKnowledgeBaseWidget->layout()->addWidget(newItem);

    adjustWidgetSize();
    m_pDeleteButton->setText(tr("Delete"));
}

void KnowledgeBaseListWidget::initKnowBaseList()
{
    // 先将布局中的控件清空
    QList<QWidget *> childWidgets = m_pHasKnowledgeBaseWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    QVector<QPair<int, QString>> files = KnowledgeBaseManager::getInstance().getKnowledgeBaseFiles();
    for (auto file : files) {
        createKnowBaseItem(file.second, file.first);
    }
}

void KnowledgeBaseListWidget::refreshKnowBaseList()
{
    qCDebug(logAIGUI) << "Refreshing knowledge base list.";
    QVector<QPair<int, QString>> files = KnowledgeBaseManager::getInstance().getKnowledgeBaseFiles();

    for (auto file : files) {
        QString filePath = file.second;
        int status = file.first;

        KnowledgeBaseProcessStatus knowStatus = KnowledgeBaseProcessStatus::Processing;
        switch (status) {
        case 0:
            knowStatus = KnowledgeBaseProcessStatus::Processing;
            break;
        case -1: // 兼容V1.0
        case 1:
            knowStatus = KnowledgeBaseProcessStatus::Succeed;
            break;
        case 2:
            knowStatus = KnowledgeBaseProcessStatus::FileError;
            break;
        case 3:
            knowStatus = KnowledgeBaseProcessStatus::ProcessingError;
            break;
        }

        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
        if (item) {
            // 存在-刷新状态
            item->setStatus(knowStatus);
            item->setFilePath(filePath);
            continue;
        }
    }
}

void KnowledgeBaseListWidget::refreshKnowBaseListOld(const QStringList &files, int status)
{
    qCDebug(logAIGUI) << "Refreshing knowledge base list (old). Files:" << files << ", Status:" << status;
    qDebug() << "onAddToServerStatusChanged: " << files << "   status: " << status;
    QVector<QPair<int, QString>> docFiles = KnowledgeBaseManager::getInstance().getKnowledgeBaseFiles();

    foreach(QString file, files) {
        QFileInfo fileInfo(file);
        QString fileName = fileInfo.fileName();

        KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
        if (!item)
            return;

        switch (status) {
        case -1: //文件错误
            item->setStatus(KnowledgeBaseProcessStatus::FileError);
            break;
        case -2: //数据错误
            item->setStatus(KnowledgeBaseProcessStatus::ProcessingError);
            break;
        case 1: //成功
            item->setStatus(KnowledgeBaseProcessStatus::Succeed);
            foreach(auto docFile, docFiles) {
                if (docFile.second.contains(fileName)) {
                    item->setFilePath(docFile.second);
                    break;
                }
            }

            break;
        case 2: //创建中
        default:
            item->setStatus(KnowledgeBaseProcessStatus::Processing);
            break;
        }
    }

    if (1 == status) {
        bool hasProcessing = false;
        auto list = m_pHasKnowledgeBaseWidget->findChildren<KnowledgeBaseItem *>();
        foreach(KnowledgeBaseItem *item, list) {
            if (item->status() == KnowledgeBaseProcessStatus::Processing) {
                hasProcessing = true;
                break;
            }
        }
    }
}

void KnowledgeBaseListWidget::resetEditButton()
{
    qCDebug(logAIGUI) << "Resetting edit button.";
    if (m_pDeleteButton->property("editMode").toBool()) {
        onEditButtonClicked();
    }
}

void KnowledgeBaseListWidget::updateDocStatus(QString &name, KnowledgeBaseProcessStatus status)
{
    qCDebug(logAIGUI) << "Updating doc status. Name:" << name << ", Status:" << status;
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + name);
    if (item) {
        item->setStatus(status);
    }
}

void KnowledgeBaseListWidget::updateFileSize(qint64 bytes)
{
    qCDebug(logAIGUI) << "Updating file size. Delta:" << bytes << ", New total:" << (m_allFileSize + bytes);
    m_allFileSize += bytes;
    m_sizeLabel->setText(QString("%1/1024M").arg(formatSize(m_allFileSize)));
}

QString KnowledgeBaseListWidget::formatSize(qint64 bytes)
{
    QString str = "-1B";
    if (bytes < 1024 * 1024)
        str =  QString("%1KB").arg(bytes / 1024.0, 0, 'f', 1);
    else
        str =  QString("%1M").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);

    //正则表达去掉小数点后为.0的情况
    QRegularExpression regExp(R"(\.0(?=[kmBKMG]))");
    QRegularExpressionMatch match = regExp.match(str);

    if (match.hasMatch()) {
        // 替换掉匹配的 ".0" 部分
        return str.replace(match.captured(0), "");
    } else {
        // 如果没有找到匹配的 ".0"，则返回原字符串
        return str;
    }
}

qint64 KnowledgeBaseListWidget::getDiskSpace(const QString &mountPoint)
{
    QProcess process;
    QStringList arguments;
    arguments << mountPoint;
    process.start("df", arguments);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList fields;

#ifdef COMPILE_ON_QT6
    QStringList lines = output.split(QRegularExpression("\\n"), Qt::SkipEmptyParts);
    if (lines.size() > 1)
        fields = lines.at(1).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList lines = output.split(QRegExp("\\n"), QString::SkipEmptyParts);
    if (lines.size() > 1)
        fields = lines.at(1).split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif

    if (fields.size() >= 5) {
        // 通常df的输出格式是：文件系统 容量 已用 可用 已用% 挂载点
        return fields.at(3).toLongLong() * 1024;
    }

    return -1;
}

void KnowledgeBaseListWidget::showTips(int x, int y)
{
    qCDebug(logAIGUI) << "Showing tips at position:" << QPoint(x, y);
    if (!m_tips) {
        m_tips = new DTipLabel(tr("This feature requires high hardware resources, and the reference benchmark configuration is: CPU Intel 11th generation i7 or above; "
                                  "Memory of 16GB or more; Having a NVIDIA graphics card and a 10 series or higher is the best option. "
                                  "If the configuration is too low, there may be issues such as lagging and inaccurate answers."));
        m_tips->setWordWrap(true);
        m_tips->setFixedWidth(320);
        m_tips->setContentsMargins(20, 15, 17, 15);
        m_tips->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_tips->setForegroundRole(DPalette::NoType);
        DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T8, QFont::Medium);
    }

    DPalette pa = m_tips->palette();
    if (DGuiApplicationHelper::LightType == DGuiApplicationHelper::instance()->themeType()) {
        pa.setColor(QPalette::WindowText, QColor(0, 0, 0, 255));
    } else {
        pa.setColor(QPalette::WindowText, QColor(255, 255, 255, 255 * 0.7));
    }
    m_tips->setPalette(pa);

    m_tips->adjustSize();

    QRect tipsRect = QRect(QPoint(x, y), m_tips->size());
    for (QScreen *screen : QGuiApplication::screens()) {
        QRect screenRect = screen->geometry();
        if (screenRect.contains(tipsRect))
            break;
        else if (screenRect.intersects(tipsRect)) {
            if (tipsRect.right() > screenRect.right()) {
                x -= (m_tips->width() + 20);
            }
            if (tipsRect.bottom() > screenRect.bottom()) {
                y -= m_tips->height();
            }
            break;
        }
    }

    m_tips->show(QPoint(x, y));
}
void KnowledgeBaseListWidget::hideTips()
{
    if (m_tips)
        m_tips->hide();
}

QString KnowledgeBaseListWidget::getTitleName()
{
    return m_pWidgetLabel->text();
}

bool KnowledgeBaseListWidget::checkEmbeddingPluginsStatus()
{
    //通过toolTip判断当前向量化插件安装情况
    return m_pAddButton->toolTip().isEmpty();
}

