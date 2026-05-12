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

namespace uos_ai {

class McpFilterComboBox : public DTK_WIDGET_NAMESPACE::DComboBox
{
    Q_OBJECT
public:
    explicit McpFilterComboBox(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

public:
    void showPopup() override;

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    DTK_WIDGET_NAMESPACE::DMenu *m_pMenu = nullptr;
};

class McpServerListWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT
public:
    explicit McpServerListWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

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
    DTK_WIDGET_NAMESPACE::DBackgroundGroup* creatServerItem(const QJsonValue &info);
    bool showRmMcpServerDlg(const QString &name);
    bool getThirdPartyMcpAgreement();
    void updateFilterWidth();
    void showItemsByFilter();

private:
    // 顶部控件
    DTK_WIDGET_NAMESPACE::DLabel *m_pTitleLabel = nullptr;
    McpFilterComboBox *m_pFilterComboBox = nullptr;
    DTK_WIDGET_NAMESPACE::DCommandLinkButton *m_pAddButton = nullptr;

    // 列表区域
    DTK_WIDGET_NAMESPACE::DWidget *m_pListWidget = nullptr;
    QVBoxLayout *m_pListLayout = nullptr;

    QMap<QString, DTK_WIDGET_NAMESPACE::DBackgroundGroup*> m_builtInItem {};
    QMap<QString, DTK_WIDGET_NAMESPACE::DBackgroundGroup*> m_thirdBuiltInItem {};
    QMap<QString, DTK_WIDGET_NAMESPACE::DBackgroundGroup*> m_customItem {};
    QJsonArray m_pServersInfo {};

    int m_currentFilter = 0; // 0:全部, 1:内置, 2:自定义
};
}

#endif // MCPSERVERLISTWIDGET_H
