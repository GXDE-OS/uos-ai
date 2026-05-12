#ifndef LLMPLUGINPROVIDER_H
#define LLMPLUGINPROVIDER_H

#include "modelprovider.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

class LLMPluginProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit LLMPluginProvider(QObject *parent = nullptr);
    ~LLMPluginProvider() override;

    AbstractModel* createModel(const ModelAccountPtr &acc) override;
};

}

#endif
