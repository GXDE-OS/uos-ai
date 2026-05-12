#ifndef PROVIDERLISTITEM_H
#define PROVIDERLISTITEM_H

#include "modelinfo.h"

#include <DWidget>
#include <DIconButton>
#include <DLabel>
#include <DBackgroundGroup>

namespace uos_ai {

class ProviderListItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit ProviderListItem(const ProviderAccount &data, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    
    void setEditMode(bool);
    QList<ModelAccountPtr> models();
    void addModelItem(class ModelSubItem *modelItem);

    inline const ProviderAccount& getData() const {
        return m_data;
    }

signals:
    void signalDeleteItem(const QString &id);
    void signalEditItem(const QString &id);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onDeleteButtonClicked();
    void removeModel(const QString &id);

private:
    void initUI();

private:
    ProviderAccount m_data;
    bool m_bInterrupt = false;

    DTK_WIDGET_NAMESPACE::DBackgroundGroup *m_groupWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_providerWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DIconButton *m_deleteButton = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_iconLabel = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_name = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_type = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_arrow = nullptr;
};

}
#endif // PROVIDERLISTITEM_H
