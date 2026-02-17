#include "knowledgebaselistitem.h"
#include "modifymodeldialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "operatinglinewidget.h"
#include "embeddingserver.h"

#include <DDialog>
#include <DDesktopServices>
#include <DSpinner>

#include <QHBoxLayout>
#include <QUrl>
#include <QDesktopServices>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

KnowledgeBaseItem::KnowledgeBaseItem(const QString &name, const QString &filePath, DWidget *parent)
    : DWidget(parent)
    , m_name(name)
    , m_filePath(filePath)
{
    qRegisterMetaType<LLMServerProxy>("LLMServerProxy");

    setObjectName("KnowledgeBaseItem_" + name);
    initUI();
    initConnect();
}

void KnowledgeBaseItem::initUI()
{
    m_pWidget = new OperatingLineWidget(this);
    m_pWidget->setName(m_name);
    m_pWidget->setBookIcon();
    m_pWidget->setModelShow(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pWidget);
    setLayout(layout);

    this->setFixedHeight(36);
}

void KnowledgeBaseItem::initConnect()
{
    connect(m_pWidget, &OperatingLineWidget::signalDeleteButtonClicked, this, &KnowledgeBaseItem::onDeleteButtonClicked);
    connect(m_pWidget, &OperatingLineWidget::signalNotDeleteButtonClicked, this, &KnowledgeBaseItem::onEditButtonClicked);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &KnowledgeBaseItem::onThemeTypeChanged);
}

void KnowledgeBaseItem::onDeleteButtonClicked()
{
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMessage(tr("Are you sure you want to delete this knowledge base file?"));
    dlg.addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);

    if (DDialog::Accepted == dlg.exec()) {
        qCInfo(logAIGUI) << "Confirmed deletion of KnowledgeBaseItem. Name:" << m_name;
        emit signalDeleteItem(m_name);
    } else {
        qCDebug(logAIGUI) << "Cancelled deletion of KnowledgeBaseItem. Name:" << m_name;
    }
}

void KnowledgeBaseItem::onEditButtonClicked(const QString &objectname)
{
    if (m_status == ProcessingError && objectname == "tipsIcon") {
        qCWarning(logAIGUI) << "Processing error, retrying createVectorIndex. FilePath:" << m_filePath;
        QStringList list;
        list << m_filePath;
        if (!EmbeddingServer::getInstance().createVectorIndex(list)) {
            qCCritical(logAIGUI) << "Failed to create vector index for file:" << m_filePath;
            return;
        }
        setStatus(Processing);
    } else {
        // QUrl dirUrl = QUrl::fromLocalFile(m_name);
        // Dtk::Widget::DDesktopServices::showFileItem(dirUrl);

        QFileInfo fileInfo(m_filePath);

        bool openOK = false;

        if (fileInfo.exists()) {
            openOK = QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
            qCInfo(logAIGUI) << "Open file result for KnowledgeBaseItem. FilePath:" << m_filePath << ", Success:" << openOK;
        } else {
            qCWarning(logAIGUI) << " file can't find:" << m_filePath;
        }
    }
}


void KnowledgeBaseItem::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    qCDebug(logAIGUI) << "Theme type changed for KnowledgeBaseItem. Name:" << m_name << ", Status:" << m_status;
    switch (m_status) {
    case Processing: {
        QString icon = QString(":/icons/deepin/builtin/%1/icons/tip.svg");
        if (DGuiApplicationHelper::LightType == themeType)
            icon = icon.arg("light");
        else
            icon = icon.arg("dark");
        m_pWidget->setTipsIcon(icon);
        break;
    }
    case ProcessingError: {
        QString icon = QString(":/icons/deepin/builtin/%1/icons/retry.svg");
        if (DGuiApplicationHelper::LightType == themeType)
            icon = icon.arg("light");
        else
            icon = icon.arg("dark");
        m_pWidget->setTipsIcon(icon);
        break;
    }
    case FileError:{
        m_pWidget->setTipsIcon("");
        break;
    }
    case Succeed:
    default: {
        m_pWidget->setTipsIcon("");
        break;
    }
    }
}

void KnowledgeBaseItem::setEditMode(bool edit)
{
    m_pWidget->setEditMode(edit);
    m_pWidget->setInterruptFilter(edit);

    update();
}

void KnowledgeBaseItem::setStatus(KnowledgeBaseProcessStatus status)
{
    m_status = status;
    m_pWidget->setStatus(m_status);
}

void KnowledgeBaseItem::setFileSize(qint64 bytes)
{
    m_fileSize = bytes;

    if (m_pWidget)
        m_pWidget->setFileSize(bytes);
}

void KnowledgeBaseItem::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}
