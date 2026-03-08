#ifndef MCPSERVERLISTWIDGET_H
#define MCPSERVERLISTWIDGET_H

#include <DWidget>
#include <DBackgroundGroup>
#include <DLabel>
#include <DComboBox>
#include <DPushButton>
#include <DCommandLinkButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

class McpFilterComboBox : public DComboBox
{
    Q_OBJECT
public:
    explicit McpFilterComboBox(DWidget *parent = nullptr);

public:
    void showPopup() override;

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    DMenu *m_pMenu = nullptr;
};

class McpServerListWidget : public DWidget
{
    Q_OBJECT
public:
    explicit McpServerListWidget(DWidget *parent = nullptr);

    void updateMcpServersInfo();

public Q_SLOTS:
    void onThemeTypeChanged();
    void onFilterChanged(int index);
    void onAddServerClicked();
    void onEditServerClicked(const QString &name);
    void removeCustomMcpServer(const QString &name);
    void onFontChanged();    
    void refreshAllItemsCheckState();

private:
    void initUI();
    void resetMcpServerItems();
    void updateServerList();
    DBackgroundGroup* creatServerItem(const QJsonValue &info);
    bool showRmMcpServerDlg(const QString &name);
    bool getThirdPartyMcpAgreement();
    void updateFilterWidth();
    void showItemsByFilter();

private:
    // 顶部控件
    DLabel *m_pTitleLabel = nullptr;
    McpFilterComboBox *m_pFilterComboBox = nullptr;
    DCommandLinkButton *m_pAddButton = nullptr;

    // 列表区域
    DWidget *m_pListWidget = nullptr;
    QVBoxLayout *m_pListLayout = nullptr;

    QMap<QString, DBackgroundGroup*> m_builtInItem {};
    QMap<QString, DBackgroundGroup*> m_thirdBuiltInItem {};
    QMap<QString, DBackgroundGroup*> m_customItem {};
    QJsonArray m_pServersInfo {};

    int m_currentFilter = 0; // 0:全部, 1:内置, 2:自定义
};
}

#endif // MCPSERVERLISTWIDGET_H
