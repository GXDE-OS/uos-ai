#ifndef KNOWLEDGEBASELISTWIDGET_H
#define KNOWLEDGEBASELISTWIDGET_H

#include <DWidget>
#include <DCommandLinkButton>
#include <DBackgroundGroup>
#include <DLabel>
#include <DTipLabel>

#include "knowledgebaselistitem.h"

DWIDGET_USE_NAMESPACE

class ThemedLable;

class KnowledgeBaseListWidget: public DWidget
{
    Q_OBJECT

public:
    explicit KnowledgeBaseListWidget(DWidget *parent = nullptr);

    void removeKnowledgeBase(const QString &name);

    void resetEditButton();
    void updateDocStatus(QString &name, KnowledgeBaseProcessStatus status);
    void updateFileSize(qint64 bytes);
    QString formatSize(qint64 bytes);
    qint64 getDiskSpace(const QString &mountPoint);

    void showTips(int x, int y);
    void hideTips();
    QString getTitleName();

    bool checkEmbeddingPluginsStatus();

private slots:
    void onEditButtonClicked();
    void onThemeTypeChanged();

public slots:
    bool onAppendKnowledgeBase(const QString &filePath, int status);
    void onAddKnowledgeBase();
    void addKnowledgeBaseFile(const QStringList &fileList);
    void onAddToServerStatusChanged(const QStringList &files, int status);
    void onIndexDeleted(const QStringList &files);
    void onFilesProcessing(const QStringList &files);
    void onEmbeddingPluginsStatusChanged(bool);
    void onRefresh();

signals:
    void signalAddKnowledgeBase();
    void sigGenPersonalFAQ();

private:
    void initUI();
    void initConnect();

    void adjustWidgetSize();

    void initKnowBaseList();
    void refreshKnowBaseList();
    void refreshKnowBaseListOld(const QStringList &files, int status);
    void createKnowBaseItem(const QString &filePath, int status);

    DWidget *noKnowledgeBaseWidget();
    DWidget *hasKnowledgeBaseWidget();

private:
    ThemedLable *m_pWidgetLabel = nullptr;
    DBackgroundGroup *m_pNoKnowledgeBaseWidget = nullptr;
    DBackgroundGroup *m_pHasKnowledgeBaseWidget = nullptr;
    DCommandLinkButton *m_pDeleteButton = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;
    QStringList m_supportedSuffix;
    QString m_lastImportPath;
    DLabel *m_tipIconLabel = nullptr;
    DLabel *m_sizeLabel = nullptr;

    DTipLabel *m_tips = nullptr;

    qint64 m_allFileSize = 0;
};

#endif // KNOWLEDGEBASELISTWIDGET_H
