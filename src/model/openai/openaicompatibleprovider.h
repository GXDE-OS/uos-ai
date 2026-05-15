#ifndef OPENAICOMPATIBLEPROVIDER_H
#define OPENAICOMPATIBLEPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief OpenaiCompatibleProvider类
 * 用于支持OpenAI兼容API的模型提供者
 */
class OpenaiCompatibleProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit OpenaiCompatibleProvider(QObject *parent = nullptr);
    virtual ~OpenaiCompatibleProvider();

    /**
     * @brief 创建OpenAI兼容的模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;
};

} // namespace uos_ai

#endif // OPENAICOMPATIBLEPROVIDER_H
