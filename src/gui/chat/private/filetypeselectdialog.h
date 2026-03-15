#ifndef FILETYPESELECTDIALOG_H
#define FILETYPESELECTDIALOG_H

#include "eparserdocument.h"

#include <DAbstractDialog>
#include <DLabel>
#include <DPushButton>
#include <DWidget>

#include <QIcon>
#include <QList>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

class FileTypeSelectDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit FileTypeSelectDialog(const QList<QIcon> &fileIcons, int fileCount, DWidget *parent = nullptr);

    EParserDocument::FileCategory getSelectedCategory() const;

signals:
    void categorySelected(EParserDocument::FileCategory category);

private:
    void initUI();
    void setupSingleFileMode();
    void setupMultiFileMode();
    QWidget *createStackedIconWidget();
    void drawIcons(QPixmap *px, const QRect &rect) const;
    void drawCount(QPixmap *px, const QRect &rect) const;

private:
    QList<QIcon> m_fileIcons;
    int m_fileCount;
    EParserDocument::FileCategory m_selectedCategory;

    DLabel *m_titleLabel = nullptr;
    DLabel *m_iconLabel = nullptr;
    DLabel *m_hintLabel = nullptr;
    DPushButton *m_materialBtn = nullptr;
    DPushButton *m_outlineBtn = nullptr;
    QVBoxLayout *m_buttonLayout = nullptr;
};

} // namespace uos_ai

#endif // FILETYPESELECTDIALOG_H
