#ifndef DEEPSEEKPROVIDER_H
#define DEEPSEEKPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief DeepSeekProvider类
 * 用于支持DeepSeek官方API的模型提供者
 */
class DeepSeekProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit DeepSeekProvider(QObject *parent = nullptr);
    ~DeepSeekProvider() override;

    /**
     * @brief 创建DeepSeek官方的模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // DEEPSEEKPROVIDER_H