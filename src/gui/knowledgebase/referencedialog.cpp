#include "referencedialog.h"
#include "uosai_global.h"
#include "utils/util.h"

#include <QVBoxLayout>
#include <QTextDocument>
#include <QFileInfo>
#include <QDesktopServices>
#include <QTextBlock>
#include <QDebug>
#include <QLoggingCategory>

#include <DWidget>
#include <DTitlebar>
#include <DLabel>
#include <DPlainTextEdit>
#include <DFontSizeManager>
#include <DHorizontalLine>
#include <DScrollArea>
#include <DPaletteHelper>
#include <DPushButton>
#include <DFileIconProvider>
#include <DFloatingMessage>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

ReferenceDialog::ReferenceDialog(QWidget *parent)
    : DAbstractDialog(parent)
{

}

ReferenceDialog::ReferenceDialog(const QString &docPath, const QStringList &docContents, DWidget *parent)
    : m_docPath(docPath)
    , m_contents(docContents)
    , DAbstractDialog(parent)
{
    installEventFilter(this);
    m_isLocalDoc = isLocalDoc(m_docPath);

    initUI();
}

void ReferenceDialog::initUI()
{
    setFixedSize(600, 500);

    DTitlebar *title = new DTitlebar(this);
    title->setMenuVisible(false);

    DLabel *titleNameLabel = new DLabel(title);
    titleNameLabel->setText((tr("Reference")));
    DFontSizeManager::instance()->bind(titleNameLabel, DFontSizeManager::T5, QFont::DemiBold);
    title->addWidget(titleNameLabel, Qt::AlignCenter);

    DScrollArea *scroll = new DScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidgetResizable(true);
    scroll->setContentsMargins(0, 0, 0, 0);
    DWidget *contentWidget = new DWidget(scroll);
    contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    contentWidget->setContentsMargins(10, 5, 5, 0);

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setAlignment(Qt::AlignTop);
    for (const QString &ref : m_contents) {
        DPlainTextEdit *content = new DPlainTextEdit(contentWidget);
        DPalette contentPalette = content->palette();
        contentPalette.setColor(DPalette::Base, Qt::transparent);
        content->setPalette(contentPalette);
        content->setContentsMargins(0, 0, 0, 0);
        content->setReadOnly(true);
        content->setFrameShape(QFrame::NoFrame);
        content->setPlainText(ref);
        content->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        content->setLineWrapMode(DPlainTextEdit::WidgetWidth);
        content->setWordWrapMode(QTextOption::WrapAnywhere);
        QFontMetrics fm(content->font());
        int lineCount = 0;
        for (int i = 0; i < content->document()->blockCount(); i++) {
            lineCount += fm.horizontalAdvance(content->document()->findBlockByNumber(i).text()) / (this->width() - 42) + 1;
        }

        int height = fm.height() * (static_cast<int>(lineCount)) + fm.capHeight();
        content->setFixedHeight(height);
        contentLayout->addWidget(content);
        if (ref == m_contents.last())
            break;
        DHorizontalLine *hLineContent = new DHorizontalLine(scroll);
        hLineContent->setLineWidth(1);
        contentLayout->addWidget(hLineContent);
    }
    contentWidget->setLayout(contentLayout);
    scroll->setWidget(contentWidget);

    DScrollArea *refSourceWidget = new DScrollArea(this);
    refSourceWidget->setFixedHeight(50);
    DLabel *srouceIconLabel = new DLabel(refSourceWidget);
    srouceIconLabel->setPixmap(getFileIcon(m_docPath).pixmap(QSize(16, 16)));
    DLabel *docPathLabel = new DLabel(parserDocName(m_docPath), refSourceWidget);
    docPathLabel->setElideMode(Qt::ElideRight);
    docPathLabel->setToolTip(parserDocName(m_docPath));

    QHBoxLayout *sourceFrameLayout = new QHBoxLayout(refSourceWidget);
    sourceFrameLayout->setAlignment(Qt::AlignLeft);
    sourceFrameLayout->setContentsMargins(21, 0, 21, 0);
    sourceFrameLayout->addWidget(srouceIconLabel);
    sourceFrameLayout->addWidget(docPathLabel);

    if(parserDocName(m_docPath) != ""){
        DPushButton *openBtn = new DPushButton((tr("Open")), refSourceWidget);
        sourceFrameLayout->addWidget(openBtn, 1, Qt::AlignRight);
        connect(openBtn, &DPushButton::clicked, this, &ReferenceDialog::onOpenBtnClicked);
    }else {
        docPathLabel->setText(m_docPath);
        docPathLabel->setToolTip(m_docPath);
        srouceIconLabel->setPixmap(QIcon(QString(":/assets/images/text-plain.png")).pixmap(QSize(16, 16)));
    }


    refSourceWidget->setLayout(sourceFrameLayout);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(scroll);
    layout->addWidget(refSourceWidget);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    this->setLayout(layout);


}

void ReferenceDialog::onOpenBtnClicked()
{
    if (m_isLocalDoc) {
        QFileInfo docInfo(m_docPath);
        if (docInfo.exists()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_docPath));
            return;
        }

        qCWarning(logAIGUI) << "Local document not found:" << m_docPath;
        DFloatingMessage *floatMessage = new DFloatingMessage(DFloatingMessage::TransientType, this);
        floatMessage->setMessage(tr("Documents don't exits!"));
        floatMessage->setBlurBackgroundEnabled(true);
        floatMessage->setDuration(800);
        floatMessage->setIcon(QIcon(":/assets/images/warning.png"));

        QRect geometry(QPoint(0, 0), floatMessage->sizeHint());
        geometry.moveCenter(rect().center());

        geometry.moveBottom(rect().bottom() - 50);
        floatMessage->setGeometry(geometry);
        floatMessage->show();
    } else {
        QDesktopServices::openUrl(m_docPath);
    }
}

bool ReferenceDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    switch (event->type()) {
    case QEvent::WindowDeactivate:
        close();
        break;
    default:
        break;
    };
    return DAbstractDialog::eventFilter(obj, event);
}

QString ReferenceDialog::parserDocName(const QString &docPath)
{
    QRegularExpression fileNameRegex("^(.*)/(.*)$");
    QString fileName;
    QRegularExpressionMatch match = fileNameRegex.match(docPath);
    if (match.hasMatch()) {
        fileName = match.captured(2);
    }

    if (!m_isLocalDoc)
        fileName = fileName.split('.')[0];

    return fileName;
}

bool ReferenceDialog::isLocalDoc(const QString &docPath)
{
    // 玩机助手使用在线wiki，做特殊处理与个人知识助手区分
    QRegularExpression regex("^https://");
    QRegularExpressionMatch match = regex.match(docPath);

    return !match.hasMatch();
}

QIcon ReferenceDialog::getFileIcon(const QString &docPath)
{
    if (docPath.isEmpty())
        return QIcon();

    QFileInfo docInfo(docPath);
    if (!docInfo.exists())
        return QIcon(QString(":/assets/images/text-html.png"));

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

    return icon;
}
