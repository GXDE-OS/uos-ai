#include "knowledgebaselistwidget.h"
//#include "modellistitem.h"
//#include "addmodeldialog.h"
#include "themedlable.h"
#include "knowledgebaselistitem.h"
#include "embeddingserver.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QProcess>

#include <DLabel>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DFileDialog>
#include <DDialog>
#include <DArrowRectangle>
#include <DFloatingWidget>

static const qint64 KnowledgeBaseSize = 1024 * 1024 * 1024;


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

    QStringList docFiles = EmbeddingServer::getInstance().getDocFiles();
    setKnowledgeBaseList(docFiles);
    foreach(const QString &file, docFiles) {
        QFileInfo fileInfo(file);
        QString name = fileInfo.fileName();
        updateDocStatus(name, KnowledgeBaseProcessStatus::Succeed);
    }
}

void KnowledgeBaseListWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(10);

    m_pDeleteButton = new DCommandLinkButton(tr("Delete"), this);
    DFontSizeManager::instance()->bind(m_pDeleteButton, DFontSizeManager::T8, QFont::Normal);
    m_pDeleteButton->setProperty("editMode", false);
    m_pDeleteButton->hide();
    m_pAddButton = new DCommandLinkButton(tr("Add"), this);
    DFontSizeManager::instance()->bind(m_pAddButton, DFontSizeManager::T8, QFont::Normal);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setMargin(0);
    topLayout->setSpacing(0);

    ThemedLable *label = new ThemedLable(tr("Knowledge Base Management"));
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T5, QFont::Medium);


    m_sizeLabel = new DLabel(this);
    m_sizeLabel->setText(QString("%1M/1024M").arg(m_allFileSize));
    DFontSizeManager::instance()->bind(m_sizeLabel, DFontSizeManager::T8, QFont::Medium);
    m_sizeLabel->setForegroundRole(DPalette::TextTips);

    QString icon = QString(":/icons/deepin/builtin/%1/icons/tip.svg");
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        icon = icon.arg("light");
    else
        icon = icon.arg("dark");
    m_tipIconLabel = new DLabel(this);
    m_tipIconLabel->setPixmap(icon);
    m_tipIconLabel->installEventFilter(this);

    topLayout->addWidget(label);
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
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::addToServerStatusChanged, this, &KnowledgeBaseListWidget::onAddToServerStatusChanged);
    connect(&EmbeddingServer::getInstance(), &EmbeddingServer::indexDeleted, this, &KnowledgeBaseListWidget::onIndexDeleted);
}

void KnowledgeBaseListWidget::onThemeTypeChanged()
{
    DPalette pl = m_pHasKnowledgeBaseWidget->palette();
    pl.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pHasKnowledgeBaseWidget->setPalette(pl);

    DPalette pl1 = m_pNoKnowledgeBaseWidget->palette();
    pl1.setBrush(DPalette::Base, DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::ItemBackground));
    m_pNoKnowledgeBaseWidget->setPalette(pl1);

    QString icon = QString(":/icons/deepin/builtin/%1/icons/tip.svg");
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        icon = icon.arg("light");
    else
        icon = icon.arg("dark");
    m_tipIconLabel->setPixmap(icon);
}

void KnowledgeBaseListWidget::onEditButtonClicked()
{
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
    layout->setMargin(0);

    m_pHasKnowledgeBaseWidget = new DBackgroundGroup(layout, this);
    m_pHasKnowledgeBaseWidget->setContentsMargins(0, 0, 0, 0);
    m_pHasKnowledgeBaseWidget->setItemSpacing(1);
    return m_pHasKnowledgeBaseWidget;
}

void KnowledgeBaseListWidget::setKnowledgeBaseList(QStringList &files)
{
    // 先将布局中的控件清空
    QList<QWidget *> childWidgets = m_pHasKnowledgeBaseWidget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets) {
        childWidget->deleteLater();
    }

    for (auto file : files) {
        onAppendKnowledgeBase(file);
    }

    adjustWidgetSize();
}

void KnowledgeBaseListWidget::removeKnowledgeBase(const QString &name)
{
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + name);
    if(!item)
        return;

    if (item->status() == KnowledgeBaseProcessStatus::Succeed) {
        QStringList fileList;
        fileList << item->filePath();
        EmbeddingServer::getInstance().deleteVectorIndex(fileList);
    } else {
        m_pHasKnowledgeBaseWidget->layout()->removeWidget(item);
        item->setParent(nullptr);
        delete item;

        adjustWidgetSize();
    }
}

void KnowledgeBaseListWidget::onAppendKnowledgeBase(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
    if (item) return;

    QFile file(filePath);
    qint64 bytes = 0;
    if (file.exists()) {
        QFileInfo fileInfo(file);
        bytes = fileInfo.size();
    }

    KnowledgeBaseItem *newItem = new KnowledgeBaseItem(fileName, filePath, this);
    newItem->setStatus(KnowledgeBaseProcessStatus::Processing);

    newItem->setFileSize(bytes);
    updateFileSize(bytes);

    connect(newItem, &KnowledgeBaseItem::signalDeleteItem, this, &KnowledgeBaseListWidget::removeKnowledgeBase, Qt::QueuedConnection);
    m_pHasKnowledgeBaseWidget->layout()->addWidget(newItem);

    adjustWidgetSize();
    m_pDeleteButton->setText(tr("Delete"));
}

bool KnowledgeBaseListWidget::onAppendKnowledgeBase(const QStringList &filePathList)
{
    bool ret = true;
    bool isFileAlreadyExist = false;
    QString fileName = "";
    for (QString filePath : filePathList) {
        QFileInfo fileInfo(filePath);
        fileName = fileInfo.fileName();
        KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
        if (item) {
            isFileAlreadyExist = true;
            break;
        }
    }

    if (isFileAlreadyExist) {
        // 已存在同名文件
        DDialog dialog(tr("File already exist") ,
                       tr("The %1 file already exists and cannot be added again. "
                          "Please modify the file name or delete the existing file before adding it again").arg(fileName));
        dialog.setIcon(QIcon(":assets/images/warning.svg"));
        dialog.addButton(QObject::tr("OK", "button"));
        dialog.exec();
        ret = false;
    } else {
        for (QString filePath : filePathList) {
            onAppendKnowledgeBase(filePath);
        }
    }

    return ret;
}
void KnowledgeBaseListWidget::onAddKnowledgeBase()
{
    DFileDialog fileDlg(this);
    fileDlg.setDirectory(m_lastImportPath);
    QString selfilter = tr("All files") + (" (%1)");
    selfilter = selfilter.arg(m_supportedSuffix.join(" "));
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(DFileDialog::ExistingFiles);
    fileDlg.setOption(DFileDialog::HideNameFilterDetails);
    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    fileDlg.setObjectName("fileDialogAdd");
    if (DFileDialog::Accepted == fileDlg.exec()) {
        m_lastImportPath = fileDlg.directory().path();
        QStringList fileList = fileDlg.selectedFiles();

        qint64 bytes = 0;
        foreach(QString filePath, fileList) {
            QFile file(filePath);
            if (file.exists()) {
                QFileInfo fileInfo(file);
                bytes += fileInfo.size();
            }
        }
        if ((bytes + m_allFileSize) > (KnowledgeBaseSize)) {
            //知识库容量不足
            DDialog dialog(QObject::tr("Insufficient knowledge base capacity")
                           , QObject::tr("The total capacity of the knowledge base is %1M, with a remaining %2. "
                                         "The total number of files added this time is %3. Unable to complete the add to knowledge base operation.")
                           .arg(1024).arg(formatSize(KnowledgeBaseSize - m_allFileSize)).arg(formatSize(bytes)));
            dialog.setIcon(QIcon(":assets/images/warning.svg"));
            dialog.addButton(QObject::tr("OK", "button"));
            int ret = dialog.exec();
            return;
        }

        qint64 diskSpaceSize = getDiskSpace(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        //diskSpaceSize = 1024;
        if ((bytes) > diskSpaceSize) {
            //磁盘容量不足
            DDialog dialog(tr("Not enough disk space") ,
                           tr("To store the newly added files, at least %1 of disk space is required. The current remaining space is %2. "
                              "Please clear enough hard disk space and try again.").arg(formatSize(bytes)).arg(formatSize(diskSpaceSize)));
            dialog.setIcon(QIcon(":assets/images/warning.svg"));
            dialog.addButton(QObject::tr("OK", "button"));
            int ret = dialog.exec();
            return;
        }

        bool appendRet = onAppendKnowledgeBase(fileList);

        for (const QString &file : fileList) {
            if (appendRet && !EmbeddingServer::getInstance().createVectorIndex(QStringList(file)))
                return;
        }
    }
}

void KnowledgeBaseListWidget::onAddToServerStatusChanged(const QStringList &files, int status)
{
    qDebug() << "onAddToServerStatusChanged: " << files << "   status: " << status;
    QStringList docFiles = EmbeddingServer::getInstance().getDocFiles();

    foreach(QString file, files) {
        QFileInfo fileInfo(file);
        QString fileName = fileInfo.fileName();

        KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + fileName);
        if (!item)
            return;

        switch (status) {
        case 0: //失败
            item->setStatus(KnowledgeBaseProcessStatus::ProcessingError);
            break;
        case 1: //成功
            item->setStatus(KnowledgeBaseProcessStatus::Succeed);
            foreach(QString docFile, docFiles) {
                if (docFile.contains(fileName)) {
                    item->setFilePath(docFile);
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
        if (!hasProcessing)
            emit sigGenPersonalFAQ();
    }
}

void KnowledgeBaseListWidget::onIndexDeleted(const QStringList &files)
{
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
}

bool KnowledgeBaseListWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        if (watched == m_tipIconLabel) {
            QPoint p = mapToGlobal(m_tipIconLabel->pos());
            showTips(p.x() + 20, p.y() + 15);
        }
    } else if (event->type() == QEvent::Leave) {
        if (watched == m_tipIconLabel) {
            hideTips();
        }

    }
    return DWidget::eventFilter(watched, event);
}

void KnowledgeBaseListWidget::adjustWidgetSize()
{
    if (m_pHasKnowledgeBaseWidget->layout()->count() <= 0) {
        m_pNoKnowledgeBaseWidget->show();
        m_pHasKnowledgeBaseWidget->hide();
        m_pDeleteButton->hide();
        m_pDeleteButton->setProperty("editMode", false);
        m_pAddButton->setEnabled(true);
    } else {
        m_pNoKnowledgeBaseWidget->hide();
        m_pHasKnowledgeBaseWidget->show();
        m_pDeleteButton->show();
    }
    adjustSize();
}

void KnowledgeBaseListWidget::resetEditButton()
{
    if (m_pDeleteButton->property("editMode").toBool()) {
        onEditButtonClicked();
    }
}

void KnowledgeBaseListWidget::updateDocStatus(QString &name, KnowledgeBaseProcessStatus status)
{
    KnowledgeBaseItem *item = m_pHasKnowledgeBaseWidget->findChild<KnowledgeBaseItem *>("KnowledgeBaseItem_" + name);
    if (item) {
        item->setStatus(status);
    }
}

void KnowledgeBaseListWidget::updateFileSize(qint64 bytes)
{
    m_allFileSize += bytes;
    m_sizeLabel->setText(QString("%1/1024M").arg(formatSize(m_allFileSize)));
}

QString KnowledgeBaseListWidget::formatSize(qint64 bytes)
{
    QString str = "-1B";

    if (bytes < 1024)
        str = QString("%1B").arg(bytes);
    else if (bytes < 1024 * 1024)
        str =  QString("%1KB").arg(bytes / 1024.0, 0, 'f', 1);
    else
        str =  QString("%1M").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);

    return str;
}

qint64 KnowledgeBaseListWidget::getDiskSpace(const QString &mountPoint)
{
    QProcess process;
    QString command = "df " + mountPoint; //单位是KB
    process.start(command);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split(QRegExp("\\n"), QString::SkipEmptyParts);

    QStringList fields = lines.at(1).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (fields.size() >= 5) {
        // 通常df的输出格式是：文件系统 容量 已用 可用 已用% 挂载点
        return fields.at(3).toLongLong() * 1024;
    }

    return -1;
}

void KnowledgeBaseListWidget::showTips(int x, int y)
{
    if (!m_tips) {
        m_tips = new DLabel();
        m_tips->setText(tr("This feature requires high hardware resources, and the reference benchmark configuration is: CPU Intel 11th generation i7 or above; "
                           "Memory of 16GB or more; Having a NVIDIA graphics card and a 10 series or higher is the best option. "
                           "If the configuration is too low, there may be issues such as lagging and inaccurate answers."));
        m_tips->setWordWrap(true);
        m_tips->setFixedWidth(320);
        m_tips->setContentsMargins(20, 15, 17, 15);
        QPalette pl = m_tips->palette();
        pl.setColor(QPalette::Text, DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Text));
        m_tips->setPalette(pl);
        m_tips->setForegroundRole(QPalette::Text);
        m_tips->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_tips->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::Window);
        m_tips->setForegroundRole(DPalette::TextTitle);
        DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T8, QFont::Medium);

        m_tips->move(x, y);
        m_tips->show();
    } else {
        m_tips->move(x, y);
        m_tips->show();
    }
}
void KnowledgeBaseListWidget::hideTips()
{
    if (m_tips)
        m_tips->hide();
}

