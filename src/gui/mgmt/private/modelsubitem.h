#ifndef MODELSUBITEM_H
#define MODELSUBITEM_H

#include "modelinfo.h"

#include <DWidget>
#include <DIconButton>
#include <DLabel>

namespace uos_ai {

class ModelSubItem : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit ModelSubItem(const ModelAccountPtr &data, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    
    inline const ModelAccountPtr& getData() const {
        return m_data;
    }
    void setEditMode(bool);
signals:
    void signalDeleteItem(const QString &id);
private slots:
    void onDeleteButtonClicked();
private:
    void initUI();

private:
    uos_ai::ModelAccountPtr m_data;
    DTK_WIDGET_NAMESPACE::DLabel *m_name = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_type = nullptr;
    DTK_WIDGET_NAMESPACE::DIconButton *m_deleteButton = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_rightSpace = nullptr;
};

}

#endif // MODELSUBITEM_H
