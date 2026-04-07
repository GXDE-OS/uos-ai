#ifndef SKILLSERVERLISTWIDGET_H
#define SKILLSERVERLISTWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DComboBox>
#include <DPushButton>
#include <DCommandLinkButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include "skillsmanager.h"

DWIDGET_USE_NAMESPACE

namespace uos_ai {

class SkillServerListWidget : public DWidget
{
    Q_OBJECT
public:
    explicit SkillServerListWidget(DWidget *parent = nullptr);

    void updateSkillServersInfo();

public Q_SLOTS:
    void onThemeTypeChanged();
    void onAddServerClicked();
    void removeCustomSkillServer(const QString &name, bool isDeletable);
    void refreshAllItemsCheckState();

private:
    void initUI();
    void resetSkillServerItems();
    void updateServerList();
    DBackgroundGroup* creatServerItem(SkillInfo &info);
    bool showRmSkillServerDlg(const QString &name);
    bool getThirdPartyMcpAgreement();

private:
    // 顶部控件
    DLabel *m_pTitleLabel = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;
    DCommandLinkButton *m_pReloadButton = nullptr;

    // 列表区域
    DWidget *m_pListWidget = nullptr;
    QVBoxLayout *m_pListLayout = nullptr;

    QList<DBackgroundGroup*> m_builtInItem {};
    QList<DBackgroundGroup*> m_thirdBuiltInItem {};
    QList<DBackgroundGroup*> m_customItem {};
    QList<SkillInfo> m_pSkillInfo {};

    QString m_lastImportPath;
    QStringList m_supSuffix;
    QString m_skillsPath;
protected:
    QScopedPointer<SkillsManager> m_skills;
};
}

#endif // SKILLSERVERLISTWIDGET_H
