#ifndef UOSFREEPROVIDER_H
#define UOSFREEPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief UosFreeProvider类
 * 用于支持UOS免费模型提供者，包括DeepSeek等模型
 */
class UosFreeProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit UosFreeProvider(QObject *parent = nullptr);
    virtual ~UosFreeProvider();

    /**
     * @brief创建UOS免费模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
protected:
    ModelAccountPtr decryptAccount(const ModelAccountPtr &acc);
};

} // namespace uos_ai

#endif // UOSFREEPROVIDER_H
