#ifndef YOURONGMODELITEM_H
#define YOURONGMODELITEM_H

#include "modelscopeitem.h"

namespace uos_ai {

class YouRongModelItem : public ModelScopeItem
{
    Q_OBJECT
public:
    explicit YouRongModelItem(bool small, DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~YouRongModelItem();

protected:
    void initUI() override;
    void onInstall() override;
};

}

#endif // YOURONGMODELITEM_H
