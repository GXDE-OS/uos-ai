#ifndef VOLCENGINEPROVIDER_H
#define VOLCENGINEPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief VolcengineProvider类
 * 用于支持豆包/火山引擎官方API的模型提供者
 */
class VolcengineProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit VolcengineProvider(QObject *parent = nullptr);
    ~VolcengineProvider() override;

    /**
     * @brief 创建豆包模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // VOLCENGINEPROVIDER_H
