#ifndef KNOWLEDGEBASELISTITEM_H
#define KNOWLEDGEBASELISTITEM_H

#include "serverdefs.h"
#include "mgmtdefs.h"

#include <DWidget>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class OperatingLineWidget;

class KnowledgeBaseItem : public DWidget
{
    Q_OBJECT

public:
    explicit KnowledgeBaseItem(const QString &name, const QString &filePath, DWidget *parent = nullptr);
    // 其他成员函数和数据成员
    void setEditMode(bool);
    void setStatus(KnowledgeBaseProcessStatus status);
    KnowledgeBaseProcessStatus status() { return m_status; }
    void setFileSize(qint64 bytes);
    qint64 fileSize() { return m_fileSize; }
    void setFilePath(const QString &filePath);
    QString filePath() { return m_filePath; }

signals:
    void signalDeleteItem(const QString &name);

private slots:
    void onEditButtonClicked();
    void onDeleteButtonClicked();
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);

private:
    void initUI();
    void initConnect();

private:
    OperatingLineWidget *m_pWidget = nullptr;
    QString m_name;
    KnowledgeBaseProcessStatus m_status;
    qint64 m_fileSize = 0;
    QString m_filePath;
};

#endif // KNOWLEDGEBASELISTITEM_H
