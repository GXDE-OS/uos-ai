#ifndef MCPSERVERLISTITEMTOOLTIP_H
#define MCPSERVERLISTITEMTOOLTIP_H

#include <DWidget>

#include <QTextBrowser>
#include <QVBoxLayout>
#include <QTimer>

namespace uos_ai {
// 自定义Tooltip类
class McpServerListItemTooltip : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit McpServerListItemTooltip(QWidget *parent = nullptr);
    void setContent(const QString &name, const QString &description);

    void hideTooltip(int delay = 0);
    void holdTooltip();

private slots:
    void onAnchorClicked(const QUrl &url);

private:
    void initUI();

    QTextBrowser *m_pTextBrowser = nullptr;
    QVBoxLayout *m_pLayout = nullptr;

    QTimer *m_pHideTimer = nullptr;
};

}

#endif // MCPSERVERLISTITEMTOOLTIP_H
