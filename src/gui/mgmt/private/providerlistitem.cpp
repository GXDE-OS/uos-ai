#include "providerlistitem.h"
#include "operatinglinewidget.h"
#include "modelsubitem.h"
#include "appdatabase.h"
#include "builtinprovider.h"
#include "global_key_define.h"
#include "model/modelvendor.h"

#include <DDialog>
#include <DGuiApplicationHelper>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLoggingCategory>
#include <QApplication>
#include <QMouseEvent>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE
using namespace uos_ai;


ProviderListItem::ProviderListItem(const ProviderAccount &data, DWidget *parent)
    : DWidget(parent)
    , m_data(data)
{
    setObjectName("ProviderListItem_" + data.id);
    initUI();
}

void ProviderListItem::initUI()
{
    BuiltinProvider::ProviderInfo providerInfo = BuiltinProvider::instance()->queryProvider(m_data.provider);
    m_providerWidget = new DWidget;

    QString typeName = providerInfo.name;
    QString providerName = m_data.name;
    if (providerName.isEmpty())
        providerName = providerInfo.name;

    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(10, 0, 10, 0);
        layout->setSpacing(10);
        m_providerWidget->setLayout(layout);
        m_providerWidget->setFixedSize(560, 36);

        m_deleteButton = new DIconButton(DStyle::SP_DeleteButton);
        m_deleteButton->setIconSize(QSize(16, 16));
        m_deleteButton->setFlat(true);
        m_deleteButton->hide();
        layout->addWidget(m_deleteButton);

        // 图标
        m_iconLabel = new DLabel;
        m_iconLabel->setFixedSize(16, 16);
        m_iconLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_bigmodel").pixmap(10, 12));
        m_iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_iconLabel);

        // 名称
        m_name = new DLabel;
        m_name->setTextFormat(Qt::PlainText);
        DFontSizeManager::instance()->bind(m_name, DFontSizeManager::T6, QFont::Medium);
        m_name->setElideMode(Qt::ElideRight);
        m_name->setMinimumWidth(20);
        m_name->setText(providerName);
        layout->addWidget(m_name);

        layout->addStretch();

        // 类型
        auto typeWidget = new DWidget;
        {
            QHBoxLayout *hlayout = new QHBoxLayout();
            hlayout->setContentsMargins(0, 0, 5, 0);
            hlayout->setSpacing(5);
            typeWidget->setLayout(hlayout);

            m_type = new DLabel;
            m_type->setTextFormat(Qt::PlainText);
            DFontSizeManager::instance()->bind(m_type, DFontSizeManager::T8, QFont::Normal);
            m_type->setElideMode(Qt::ElideRight);
            m_type->setForegroundRole(DPalette::TextTips);
            m_type->setMinimumWidth(20);
            m_type->setText(typeName);
            hlayout->addWidget(m_type);

            // 箭头
            m_arrow = new DLabel;
            {
                QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(12, 12);
                QPainter painter(&pixmap);
                painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().textTips());
                m_arrow->setPixmap(pixmap);
            }
            hlayout->addWidget(m_arrow);
        }

        layout->addWidget(typeWidget);
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    layout->addWidget(m_providerWidget);

    m_groupWidget = new DBackgroundGroup(layout, this);
    m_groupWidget->setContentsMargins(0, 0, 0, 0);
    m_groupWidget->setItemSpacing(1);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_groupWidget);
    setLayout(rootLayout);

    connect(m_deleteButton, &DIconButton::clicked, this, &ProviderListItem::onDeleteButtonClicked);
    m_providerWidget->installEventFilter(this);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this](){
        m_iconLabel->setPixmap(QIcon::fromTheme("uos-ai-assistant_bigmodel").pixmap(10, 12));

        QPixmap pixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowForward).pixmap(12, 12);
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), DGuiApplicationHelper::instance()->applicationPalette().textTips());
        m_arrow->setPixmap(pixmap);
    });
}

void ProviderListItem::onDeleteButtonClicked()
{
    qCInfo(logAIGUI) << "Delete button clicked. id:" << m_data.id;
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMessage(tr("Are you sure you want to delete this provider?"));
    dlg.addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);

    if (DDialog::Accepted == dlg.exec()) {
        qCInfo(logAIGUI) << "Confirmed delete provider. id:" << m_data.id;

        AppDatabase::instance()->deleteProvider(m_data.id);
        ModelVendor::instance()->removeProvider(m_data.id);

        emit signalDeleteItem(m_data.id);
    } else {
        qCDebug(logAIGUI) << "Cancel delete provider. id:" << m_data.id;
    }
}

void ProviderListItem::removeModel(const QString &id)
{
    auto item = m_groupWidget->findChild<ModelSubItem *>("ModelSubItem_" + id);
    if (!item) return;

    layout()->removeWidget(item);
    item->setParent(nullptr);
    delete item;

    adjustSize();
    qCInfo(logAIGUI) << "model removed. id:" << id;
}

void ProviderListItem::setEditMode(bool edit)
{
    m_deleteButton->setVisible(edit);
    m_bInterrupt = edit;

    // 免费账号不允许单独删除模型
    auto modelItems = this->findChildren<ModelSubItem *>();
    for (auto modelItem : modelItems)
        modelItem->setEditMode(edit);

    update();
    qCDebug(logAIGUI) << "Set edit mode for ProviderListItem. id:" << m_data.id << ", edit:" << edit;
}

QList<ModelAccountPtr> ProviderListItem::models()
{
    QList<ModelAccountPtr> ret;
    auto items = this->findChildren<ModelSubItem *>();
    for (auto item : items) {
        ret.append(item->getData());
    }

    return ret;
}

void ProviderListItem::addModelItem(ModelSubItem *modelItem)
{
    if (!modelItem)
        return;

    auto layout = m_groupWidget->layout();
    layout->addWidget(modelItem);
    connect(modelItem, &ModelSubItem::signalDeleteItem, this, &ProviderListItem::removeModel, Qt::QueuedConnection);
}

bool ProviderListItem::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_providerWidget && event->type() == QEvent::MouseButtonPress && !m_bInterrupt) {
        auto mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            emit signalEditItem(m_data.id);
        }
    }

    return DWidget::eventFilter(obj, event);
}
