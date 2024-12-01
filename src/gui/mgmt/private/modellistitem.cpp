#include "modellistitem.h"
#include "modifymodeldialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "operatinglinewidget.h"

#include <QHBoxLayout>
#include <DDialog>

static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

ModelListItem::ModelListItem(const LLMServerProxy &data, DWidget *parent)
    : DWidget(parent)
    , m_data(data)
{
    qRegisterMetaType<LLMServerProxy>("LLMServerProxy");

    setObjectName("ModelListItem_" + data.id);
    initUI();
    initConnect();
}

void ModelListItem::initUI()
{
    m_pWidget = new OperatingLineWidget(this);
    m_pWidget->setName(m_data.name.isEmpty() ? LLMServerProxy::llmName(m_data.model, !m_data.url.isEmpty()) : m_data.name);
    m_pWidget->setEditText(LLMServerProxy::llmName(m_data.model, !m_data.url.isEmpty()));
    m_pWidget->setModelShow(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pWidget);
    setLayout(layout);

    this->setFixedHeight(36);
}

void ModelListItem::initConnect()
{
    connect(m_pWidget, &OperatingLineWidget::signalDeleteButtonClicked, this, &ModelListItem::onDeleteButtonClicked);
    connect(m_pWidget, &OperatingLineWidget::signalNotDeleteButtonClicked, this, &ModelListItem::onEditButtonClicked);
}

void ModelListItem::onDeleteButtonClicked()
{
    DDialog dlg(this);
    dlg.setIcon(QIcon(WARNING_ICON));
    dlg.setMessage(tr("Are you sure you want to delete this model?"));
    dlg.addButton(tr("Cancel", "button"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm", "button"), true, DDialog::ButtonRecommend);

    if (DDialog::Accepted == dlg.exec()) {
        DbWrapper::localDbWrapper().deleteLlm(m_data.id);
        ServerWrapper::instance()->updateLLMAccount();
        emit signalDeleteItem(m_data);
    }
}

void ModelListItem::onEditButtonClicked()
{
    ModifyModelDialog dlg(m_data, this);
    if (QDialog::Accepted == dlg.exec()) {
        if (dlg.getModelName() != m_data.name) {
            m_data.name = dlg.getModelName();
            m_pWidget->setName(m_data.name.isEmpty() ? LLMServerProxy::llmName(m_data.model, !m_data.url.isEmpty()) : m_data.name);
            DbWrapper::localDbWrapper().updateLlm(m_data);
            ServerWrapper::instance()->updateLLMAccount();
        }
    }
}

void ModelListItem::setEditMode(bool edit)
{
    m_pWidget->setEditMode(edit);
    m_pWidget->setInterruptFilter(edit);

    update();
}
