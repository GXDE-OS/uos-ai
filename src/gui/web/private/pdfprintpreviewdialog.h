#ifndef PDFPRINTPREVIEWDIALOG_H
#define PDFPRINTPREVIEWDIALOG_H

#include <QtGlobal>

#ifdef COMPILE_ON_QT6
#include <poppler-qt6.h>
#include <memory>
#else
#include <poppler-qt5.h>
#endif

#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QVector>
#include <QImage>
#include <QSize>
#include <QString>

#include <DPrintPreviewDialog>
#include <dprintpreviewwidget.h>

namespace uos_ai {

class PDFPrintPreviewDialog : public QObject
{
    Q_OBJECT

public:
    explicit PDFPrintPreviewDialog(const QString &pdfPath, const QString &docName = QString(), QWidget *parent = nullptr);

    bool isValid() const;
    int exec();

private slots:
    void handlePaintRequested(Dtk::Widget::DPrinter *printer);
    void handlePaintRequested(Dtk::Widget::DPrinter *printer, const QVector<int> &pageRange);

private:
    bool loadDocument(const QString &path);
    bool paintPages(Dtk::Widget::DPrinter *printer, const QVector<int> &pages);
    QImage renderPage(int pageIndex, const QSize &targetSizePx, int dpi) const;
    QVector<int> normalizePages(const QVector<int> &pages) const;

private:
    QString m_pdfPath;
#ifdef COMPILE_ON_QT6
    std::unique_ptr<Poppler::Document> m_document;
#else
    QScopedPointer<Poppler::Document> m_document;
#endif
    QPointer<Dtk::Widget::DPrintPreviewDialog> m_dialog;
};

} // namespace uos_ai

#endif // PDFPRINTPREVIEWDIALOG_H
