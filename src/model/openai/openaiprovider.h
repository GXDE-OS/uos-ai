#ifndef OPENAIPROVIDER_H
#define OPENAIPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief OpenaiProvider类
 * 用于支持OpenAI官方API的模型提供者
 */
class OpenaiProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit OpenaiProvider(QObject *parent = nullptr);
    ~OpenaiProvider() override;

    /**
     * @brief 创建OpenAI官方的模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // OPENAIPROVIDER_H
