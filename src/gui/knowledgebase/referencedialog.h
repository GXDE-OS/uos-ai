#ifndef REFERENCEDIALOG_H
#define REFERENCEDIALOG_H

#include <QObject>

#include <DWidget>
#include <DAbstractDialog>

namespace uos_ai {
class ReferenceDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    explicit ReferenceDialog(QWidget *parent = nullptr);
    ReferenceDialog(const QString &docPath, const QStringList &docContents, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    void initUI();

public slots:
    void onOpenBtnClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QString parserDocName(const QString &docPath);
    QIcon getFileIcon(const QString &docPath);
    bool isLocalDoc(const QString &docPath);

    QString m_docPath;
    QStringList m_contents;
    bool m_isLocalDoc {false};
};
}

#endif // REFERENCEDIALOG_H
