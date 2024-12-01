#ifndef KNOWLEDGEBASELISTWIDGET_H
#define KNOWLEDGEBASELISTWIDGET_H

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>
#include <DLabel>

#include "knowledgebaselistitem.h"

DWIDGET_USE_NAMESPACE

class KnowledgeBaseListWidget: public DWidget
{
    Q_OBJECT

public:
    explicit KnowledgeBaseListWidget(DWidget *parent = nullptr);

    void setKnowledgeBaseList(QStringList &files);
    void removeKnowledgeBase(const QString &name);

    void resetEditButton();
    void updateDocStatus(QString &name, KnowledgeBaseProcessStatus status);
    void updateFileSize(qint64 bytes);
    QString formatSize(qint64 bytes);
    qint64 getDiskSpace(const QString &mountPoint);

    void showTips(int x, int y);
    void hideTips();

private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();

public slots:
    void onAppendKnowledgeBase(const QString &filePath);
    bool onAppendKnowledgeBase(const QStringList &filePathList);
    void onAddKnowledgeBase();
    void onAddToServerStatusChanged(const QStringList &files, int status);
    void onIndexDeleted(const QStringList &files);

signals:
    void signalAddKnowledgeBase();
    void sigGenPersonalFAQ();


protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUI();
    void initConnect();

    void adjustWidgetSize();

    DWidget *noKnowledgeBaseWidget();
    DWidget *hasKnowledgeBaseWidget();

private:
    DBackgroundGroup *m_pNoKnowledgeBaseWidget = nullptr;
    DBackgroundGroup *m_pHasKnowledgeBaseWidget = nullptr;
    DCommandLinkButton *m_pDeleteButton = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;
    QStringList m_supportedSuffix;
    QString m_lastImportPath;
    DLabel *m_tipIconLabel = nullptr;
    DLabel *m_sizeLabel = nullptr;

    DLabel *m_tips = nullptr;

    qint64 m_allFileSize = 0;
};

#endif // KNOWLEDGEBASELISTWIDGET_H
