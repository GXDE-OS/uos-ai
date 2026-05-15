#ifndef MOONSHOTPROVIDER_H
#define MOONSHOTPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief MoonshotProvider类
 * 用于支持Moonshot(KIMI)官方API的模型提供者
 */
class MoonshotProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit MoonshotProvider(QObject *parent = nullptr);
    ~MoonshotProvider() override;

    /**
     * @brief 创建Moonshot官方的模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // MOONSHOTPROVIDER_H
