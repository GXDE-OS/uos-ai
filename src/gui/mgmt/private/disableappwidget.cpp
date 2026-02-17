#include "disableappwidget.h"
#include "disableappitem.h"
#include "utils/apputils.h"

#include <DFontSizeManager>
#include <DPushButton>
#include <DLabel>
#include <DFrame>
#include <DIconButton>
#include <DGuiApplicationHelper>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QScrollArea>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>

#include <cmath>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DisableAppWidget::DisableAppWidget(DWidget *parent)
    : DWidget(parent)
{
    initUI();
    initConnect();
}

DisableAppWidget::~DisableAppWidget()
{
}

void DisableAppWidget::initUI()
{
    // 设置焦点策略为NoFocus，避免意外获得焦点
    setFocusPolicy(Qt::NoFocus);

    m_pTitleLabel = new DLabel;
    m_pTitleLabel->setText(tr("Hide FollowAlong in the following applications"));
    DFontSizeManager::instance()->bind(m_pTitleLabel, DFontSizeManager::T6, QFont::Medium);
    m_pTitleLabel->setElideMode(Qt::ElideRight);

    // 创建滚动区域内容容器
    m_scrollContent = new DWidget();
    m_scrollContent->setContentsMargins(0, 0, 0, 0);

    // 创建网格布局
    m_gridLayout = new QGridLayout(m_scrollContent);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(10);

    // 创建滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidget(m_scrollContent);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWindowFlags(Qt::FramelessWindowHint);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    m_scrollArea->setLineWidth(0);
    m_scrollArea->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 10, 0, 0);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(m_pTitleLabel);
    mainLayout->addWidget(m_scrollArea);
    mainLayout->addStretch();

    // 初始时没有禁用应用，列表为空
    m_disableAppList.clear();
    updateLayout();
}

void DisableAppWidget::initConnect()
{
    // 连接删除应用的信号
    connect(this, &DisableAppWidget::requestRemoveApp, this, &DisableAppWidget::removeDisableApp);
}

void DisableAppWidget::removeDisableApp(const QString &appName)
{
    // 记录当前鼠标位置，在删除前记录
    QPoint globalMousePos = QCursor::pos();

    // 从列表中移除应用
    m_disableAppList.removeAll(appName);

    // 检查是否列表已空
    if (m_disableAppList.isEmpty()) {
        qCInfo(logAIGUI) << "Disabled apps list is now empty";
        // 在发送信号前，将焦点设置到父窗口，避免焦点残留问题
        if (parentWidget()) {
            parentWidget()->setFocus();
        } else if (window()) {
            window()->setFocus();
        }

        // 如果列表为空，发送信号
        emit becameEmpty();
    } else {
        qCDebug(logAIGUI) << "Disabled app removed, updating layout. Remaining count:" << m_disableAppList.size();
        // 重新布局
        updateLayout();

        // 使用QTimer延迟检查鼠标位置下的部件，确保布局已经完全更新
        QTimer::singleShot(100, this, [this, globalMousePos]() {
            checkMouseHoverOnItems(globalMousePos);
        });
    }

    // 发送更新后的应用列表
    emit requestDisabledAppsUpdate(getAppInfos());
}

void DisableAppWidget::checkMouseHoverOnItems(const QPoint &globalPos)
{
    // 遍历所有新创建的项目部件
    for (QWidget *widget : m_itemWidgets) {
        DisableAppItem *item = qobject_cast<DisableAppItem*>(widget);
        if (item) {
            // 检查鼠标是否在这个项目的区域内
            QPoint itemGlobalTopLeft = item->mapToGlobal(QPoint(0, 0));
            QRect itemGlobalRect(itemGlobalTopLeft, item->size());

            if (itemGlobalRect.contains(globalPos)) {
                // 模拟鼠标进入事件
                QEvent enterEvent(QEvent::Enter);
                QApplication::sendEvent(item, &enterEvent);

                // 确保删除按钮可见
                item->setProperty("isHovered", true);

                // 找到一个项目后就可以停止遍历
                break;
            }
        }
    }
}

void DisableAppWidget::clearLayout()
{
    // 清理现有的项目部件
    for (QWidget *widget : m_itemWidgets) {
        if (widget) {
            m_gridLayout->removeWidget(widget);
            widget->deleteLater();
        }
    }
    m_itemWidgets.clear();

    // 清理可能存在的间隔器和其他布局项目
    while (QLayoutItem *item = m_gridLayout->takeAt(0)) {
        if (item->spacerItem()) {
            delete item;
        }
    }
}

void DisableAppWidget::updateLayout()
{
    // 清理现有布局
    clearLayout();

    int columnCount = 2;
    int rowCount = std::ceil(static_cast<double>(m_disableAppList.size()) / columnCount);

    // 添加应用项到网格布局
    for (int i = 0; i < m_disableAppList.size(); ++i) {
        const QString &appName = m_disableAppList.at(i);

        // 获取显示名称和图标
        QString displayName = getAppDisplayName(appName);
        QString iconName = getAppIconName(appName);

        // 确保有有效的图标
        if (iconName.isEmpty()) {
            iconName = "uos-ai-assistant_app"; // 默认图标
        }

        // 使用新的DisableAppItem类，传递显示名称、图标和原始应用名称
        DisableAppItem *item = new DisableAppItem(displayName, iconName, appName);

        // 确保项目能正确响应悬停事件
        item->setAttribute(Qt::WA_Hover, true);

        // 连接删除按钮信号
        connect(item, &DisableAppItem::deleteClicked, this, &DisableAppWidget::requestRemoveApp);

        // 计算网格位置
        int row = i / columnCount;
        int col = i % columnCount;

        // 添加到网格布局
        m_gridLayout->addWidget(item, row, col);

        // 保存部件引用以便后续清理
        m_itemWidgets.append(item);
    }

    // 调整网格布局的尺寸
    adjustGridLayout();
}

void DisableAppWidget::adjustGridLayout()
{
    // 设置ScrollContent的大小
    int columnCount = 2;
    int rowCount = std::ceil(static_cast<double>(m_disableAppList.size()) / columnCount);

    // 限制最大显示行数为3行，超过则出现滚动条
    int visibleRows = qMin(rowCount, 3);

    // 确保至少显示一行
    visibleRows = qMax(visibleRows, 1);

    // 计算内容高度
    int itemHeight = 52;
    int spacing = 10; // 使用固定的间距值，与网格布局的设置保持一致
    int contentHeight = visibleRows * itemHeight + (visibleRows - 1) * spacing +
                        m_scrollContent->contentsMargins().top() +
                        m_scrollContent->contentsMargins().bottom();

    // 计算内容宽度
    int itemWidth = 275;
    int contentWidth = columnCount * itemWidth + (columnCount - 1) * spacing +
                      m_scrollContent->contentsMargins().left() +
                      m_scrollContent->contentsMargins().right();

    // 调整宽度，为垂直滚动条预留空间而不出现水平滚动条
    // 注意: 这个值需要根据实际滚动条宽度进行微调
    contentWidth += 6; // 增加一点额外宽度确保垂直滚动条紧贴右侧

    // 设置滚动区域的固定高度
    m_scrollArea->setFixedHeight(contentHeight);

    // 设置内容区域的最小宽度
    m_scrollContent->setMinimumWidth(contentWidth);

    // 处理只有一个项目的特殊情况，确保布局左对齐
    if (m_disableAppList.size() == 1) {
        // 使用特殊方法强制左对齐
        m_gridLayout->setHorizontalSpacing(0);
        m_gridLayout->setColumnMinimumWidth(0, itemWidth);

        // 关键：设置左侧列的对齐方式和拉伸因子
        m_gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        for (QWidget *widget : m_itemWidgets) {
            if (widget) {
                m_gridLayout->setAlignment(widget, Qt::AlignLeft | Qt::AlignTop);
            }
        }

        // 添加一个宽度填充到右侧，确保不会导致水平滚动条
        QSpacerItem *hSpacer = new QSpacerItem(contentWidth - itemWidth - 15, 10,
                                               QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_gridLayout->addItem(hSpacer, 0, 1, 1, 1);
    } else {
        // 重置对齐方式和间距
        m_gridLayout->setHorizontalSpacing(10);
        m_gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    }
}

void DisableAppWidget::addApp(const QString &appName)
{
    // 检查应用是否已存在
    if (m_disableAppList.contains(appName)) {
        qCWarning(logAIGUI) << "App already in disabled list, skip adding:" << appName;
        return;
    }

    // 添加新应用到列表
    m_disableAppList.append(appName);

    // 如果之前是空的，需要先显示整个部件
    bool wasEmpty = (m_disableAppList.size() == 1);
    if (wasEmpty) {
        // 确保部件可见
        this->setVisible(true);
        if (parent() && parent()->parent()) {
            QWidget *parentWidget = qobject_cast<QWidget*>(parent()->parent());
            if (parentWidget) {
                parentWidget->setVisible(true);
            }
        }
    }

    // 发出信号通知应用已添加
    emit appAdded(appName);
}

void DisableAppWidget::clearApps()
{
    // 先清空内部数据
    m_disableAppList.clear();

    // 然后更新界面
    updateLayout();

    // 如果清空后应该隐藏
    if (m_disableAppList.isEmpty()) {
        qCInfo(logAIGUI) << "All disabled apps cleared, list is empty.";
        emit becameEmpty();
    }
}

QString DisableAppWidget::getAppDisplayName(const QString &appName)
{
    QString displayName = AppUtils::instance().getNameByApp(appName);
    if (displayName.isEmpty()) {
        qCWarning(logAIGUI) << "Display name not found for app:" << appName << ", using appName as display name.";
        return appName;
    }
    return displayName;
}

QString DisableAppWidget::getAppIconName(const QString &appName)
{
    QString iconName = AppUtils::instance().getIconByApp(appName);
    if (iconName.isEmpty()) {
        qCWarning(logAIGUI) << "Icon name not found for app:" << appName << ", using default icon.";
        return "uos-ai-assistant_app";
    }
    return processIconName(iconName);
}

QString DisableAppWidget::processIconName(const QString &iconName)
{
    QStringList iconList = iconName.split("||");
    if (iconList.isEmpty())
        iconList << iconName;

    for (const QString &icon : iconList) {
        if (!icon.startsWith("/") && icon.contains(".")) {
            int dotIndex = icon.lastIndexOf(".");
            if (dotIndex > 0) {
                if (!QIcon::fromTheme(icon.left(dotIndex)).isNull())
                    return icon.left(dotIndex);
            }
        } else {
            QFileInfo iconFile(icon);
            if (iconFile.exists())
                return icon;
        }
    }
    return QString();
}
