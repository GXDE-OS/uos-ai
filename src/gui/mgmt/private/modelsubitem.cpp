#include "modelsubitem.h"
#include "operatinglinewidget.h"
#include "builtinprovider.h"
#include "modelvendor.h"
#include "appdatabase.h"

#include <DDialog>
#include <QHBoxLayout>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE

using namespace uos_ai;

ModelSubItem::ModelSubItem(const ModelAccountPtr &data, DWidget *parent)
    : DWidget(parent)
    , m_data(data)
{
    setObjectName("ModelSubItem_" + data->id);
    initUI();
}

void ModelSubItem::setEditMode(bool b)
{
    m_type->setVisible(!b);
    m_rightSpace->setVisible(b);

    update();
}

void ModelSubItem::onDeleteButtonClicked()
{
    qCInfo(logAIGUI) << "Delete button clicked. id:" << m_data->id;
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMessage(tr("Are you sure you want to delete this model?"));
    dlg.addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);

    if (DDialog::Accepted == dlg.exec()) {
        qCInfo(logAIGUI) << "Confirmed delete model. id:" << m_data->id;
        AppDatabase::instance()->deleteModel(m_data->id);
        ModelVendor::instance()->removeModel(m_data->id);
        emit signalDeleteItem(m_data->id);
    } else {
        qCDebug(logAIGUI) << "Cancel delete model. id:" << m_data->id;
    }
}

void ModelSubItem::initUI()
{
    setFixedSize(560, 36);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(36, 0, 10, 0);

    if (!m_data->model.id.isEmpty())
        m_data->model = BuiltinProvider::instance()->getModelInfo(m_data->model.id);

    QString typeName = m_data->model.modelId;
    QString modelName = m_data->model.name;
    
    m_name = new DLabel;
    m_name->setTextFormat(Qt::PlainText);
    DFontSizeManager::instance()->bind(m_name, DFontSizeManager::T6, QFont::Medium);
    m_name->setElideMode(Qt::ElideRight);
    m_name->setMinimumWidth(20);
    m_name->setText(modelName);
    layout->addWidget(m_name);

    layout->addStretch();

    m_type = new DLabel;
    m_type->setTextFormat(Qt::PlainText);
    DFontSizeManager::instance()->bind(m_type, DFontSizeManager::T8, QFont::Normal);
    m_type->setElideMode(Qt::ElideRight);
    m_type->setForegroundRole(DPalette::TextTips);
    m_type->setMinimumWidth(20);
    m_type->setText(typeName);
    layout->addWidget(m_type);

    m_deleteButton = new DIconButton(this);
    m_deleteButton->setIcon(QIcon::fromTheme("uos-ai-assistant_delete"));
    m_deleteButton->setFixedSize(12, 14);
    m_deleteButton->setIconSize(QSize(12, 14)); // 设置图标大小
    m_deleteButton->setFlat(true); // 设置为扁平样式
    m_deleteButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_deleteButton, &DIconButton::clicked, this, &ModelSubItem::onDeleteButtonClicked);
    {
        m_rightSpace = new DWidget;
        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->setContentsMargins(0, 0, 5, 0);
        hlayout->addWidget(m_deleteButton);
        m_rightSpace->setLayout(hlayout);
        m_rightSpace->hide();
    }

    layout->addWidget(m_rightSpace);

    setLayout(layout);
}

