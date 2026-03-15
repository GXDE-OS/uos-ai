#include "pdfprintpreviewdialog.h"
#include <qglobal.h>

#include <QDialog>
#include <QFileInfo>
#include <QPageLayout>
#include <QPainter>
#include <QScopedPointer>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

namespace uos_ai {

PDFPrintPreviewDialog::PDFPrintPreviewDialog(const QString &pdfPath,
                                             const QString &docName,
                                             QWidget *parent)
    : QObject(parent), m_pdfPath(pdfPath), m_dialog(new DPrintPreviewDialog(parent))
{
    qCInfo(logAIGUI) << "Creating PDF print preview dialog for:" << pdfPath;

    if (!m_dialog) {
        qCWarning(logAIGUI) << "Failed to create DPrintPreviewDialog";
        return;
    }

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!docName.isEmpty()) {
        m_dialog->setDocName(docName);
    } else {
        m_dialog->setDocName(QFileInfo(pdfPath).fileName());
    }

    const bool ok = loadDocument(pdfPath);
    if (!ok) {
        qCWarning(logAIGUI) << "Failed to load PDF document:" << pdfPath;
        return;
    }

    qCInfo(logAIGUI) << "PDF document loaded successfully, pages:" << m_document->numPages();

    QObject::connect(m_dialog,
                     QOverload<DPrinter *>::of(&DPrintPreviewDialog::paintRequested),
                     this,
                     QOverload<DPrinter *>::of(&PDFPrintPreviewDialog::handlePaintRequested));
    QObject::connect(m_dialog,
                     QOverload<DPrinter *, const QVector<int> &>::of(
                             &DPrintPreviewDialog::paintRequested),
                     this,
                     QOverload<DPrinter *, const QVector<int> &>::of(
                             &PDFPrintPreviewDialog::handlePaintRequested));
}

bool PDFPrintPreviewDialog::isValid() const
{
    return m_dialog && m_document;
}

int PDFPrintPreviewDialog::exec()
{
    if (!isValid()) {
        return QDialog::Rejected;
    }

    return m_dialog->exec();
}

bool PDFPrintPreviewDialog::loadDocument(const QString &path)
{
    if (path.isEmpty()) {
        qCWarning(logAIGUI) << "Cannot load document: empty path";
        return false;
    }

    qCDebug(logAIGUI) << "Loading PDF document from:" << path;

#ifdef COMPILE_ON_QT6
    m_document = Poppler::Document::load(path);
#else
    m_document.reset(Poppler::Document::load(path));
#endif
    if (!m_document) {
        qCWarning(logAIGUI) << "Failed to load PDF document:" << path;
        return false;
    }

    m_document->setRenderHint(Poppler::Document::Antialiasing, true);
    m_document->setRenderHint(Poppler::Document::TextAntialiasing, true);
    return true;
}

void PDFPrintPreviewDialog::handlePaintRequested(Dtk::Widget::DPrinter *printer)
{
    if (!printer) {
        qCWarning(logAIGUI) << "Paint requested with null printer";
        return;
    }

    QVector<int> pages;
    const int totalPages = m_document ? m_document->numPages() : 0;
    for (int i = 0; i < totalPages; ++i) {
        pages.append(i);
    }

    qCInfo(logAIGUI) << "Handling paint request for all pages, total:" << totalPages;
    paintPages(printer, pages);
}

void PDFPrintPreviewDialog::handlePaintRequested(Dtk::Widget::DPrinter *printer, const QVector<int> &pageRange)
{
    if (!printer) {
        qCWarning(logAIGUI) << "Paint requested with null printer";
        return;
    }

    qCInfo(logAIGUI) << "Handling paint request for page range:" << pageRange;
    paintPages(printer, normalizePages(pageRange));
}

QVector<int> PDFPrintPreviewDialog::normalizePages(const QVector<int> &pages) const
{
    QVector<int> normalized;
    if (!m_document) {
        return normalized;
    }

    const int totalPages = m_document->numPages();
    for (int page : pages) {
        if (page >= 0 && page < totalPages) {
            normalized.append(page);
        }
    }

    if (normalized.isEmpty()) {
        for (int i = 0; i < totalPages; ++i) {
            normalized.append(i);
        }
    }

    return normalized;
}

bool PDFPrintPreviewDialog::paintPages(Dtk::Widget::DPrinter *printer,
                                       const QVector<int> &pages)
{
    if (!m_document || !printer || pages.isEmpty()) {
        qCWarning(logAIGUI) << "Cannot paint pages - document:" << (m_document != nullptr)
                            << "printer:" << (printer != nullptr)
                            << "pages empty:" << pages.isEmpty();
        return false;
    }

    qCDebug(logAIGUI) << "Painting" << pages.size() << "pages at" << printer->resolution() << "DPI";

    QPainter painter(printer);
    const QRect paintRect =
            printer->pageLayout().paintRectPixels(printer->resolution());
    const QSize targetSize = paintRect.size();
    const int dpi = printer->resolution();

    for (int pageIndex : pages) {
        if (!painter.isActive()) {
            painter.begin(printer);
        }

        const QImage image = renderPage(pageIndex, targetSize, dpi);
        if (image.isNull()) {
            qCWarning(logAIGUI) << "Failed to render page:" << pageIndex;
            continue;
        }

        QRect targetRect = QRect(QPoint(0, 0), targetSize);
        QSize scaledSize = image.size();
        scaledSize.scale(targetSize, Qt::KeepAspectRatio);
        targetRect.setSize(scaledSize);
        targetRect.moveCenter(paintRect.center());

        painter.drawImage(targetRect, image);

        if (pageIndex != pages.last()) {
            printer->newPage();
        }
    }

    painter.end();
    qCInfo(logAIGUI) << "Successfully painted" << pages.size() << "pages";
    return true;
}

QImage PDFPrintPreviewDialog::renderPage(int pageIndex,
                                         const QSize &targetSizePx,
                                         int dpi) const
{
    if (!m_document) {
        qCWarning(logAIGUI) << "Cannot render page: document is null";
        return QImage();
    }

#ifdef COMPILE_ON_QT6
    std::unique_ptr<Poppler::Page> page = m_document->page(pageIndex);
#else
    QScopedPointer<Poppler::Page> page(m_document->page(pageIndex));
#endif
    if (!page) {
        qCWarning(logAIGUI) << "Cannot get page at index:" << pageIndex;
        return QImage();
    }

    QImage image = page->renderToImage(dpi, dpi);
    if (image.isNull()) {
        return image;
    }

    if (!targetSizePx.isEmpty() && image.size() != targetSizePx) {
        image = image.scaled(targetSizePx, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    return image;
}

}   // namespace uos_ai
