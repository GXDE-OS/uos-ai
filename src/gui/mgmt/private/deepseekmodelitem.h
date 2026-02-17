#ifndef DEEPSEEKMODELITEM_H
#define DEEPSEEKMODELITEM_H

#include "modelscopeitem.h"

namespace uos_ai {

class DeepSeekModelItem : public ModelScopeItem
{
    Q_OBJECT
public:
    explicit DeepSeekModelItem(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~DeepSeekModelItem();

protected:
    void initUI() override;
    void onInstall() override;
};

}

#endif // DEEPSEEKMODELITEM_H