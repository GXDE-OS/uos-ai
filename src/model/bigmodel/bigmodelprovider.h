#ifndef BIGMODELPROVIDER_H
#define BIGMODELPROVIDER_H

#include "modelprovider.h"
#include "modelinfo.h"

#include <QObject>

namespace uos_ai {

class AbstractModel;

/**
 * @brief BigModelProvider类
 * 用于支持BigModel(智谱AI)官方API的模型提供者
 */
class BigModelProvider : public ModelProvider
{
    Q_OBJECT
public:
    explicit BigModelProvider(QObject *parent = nullptr);
    ~BigModelProvider() override;

    /**
     * @brief 创建BigModel官方的模型实例
     * @param acc 模型账户信息
     * @return 返回创建的模型实例指针
     */
    AbstractModel* createModel(const ModelAccountPtr &acc) override;

    static QString host();
};

} // namespace uos_ai

#endif // BIGMODELPROVIDER_H