#ifndef BUILTINPROVIDER_H
#define BUILTINPROVIDER_H

#include "modelinfo.h"

#include <QObject>
#include <QMap>
#include <QList>

namespace uos_ai {

inline constexpr char UOS_EXTERNAL_MODEL_PASSWD[] = "1baddd9028c84e9d";
inline constexpr char UOS_FREE_MODEL_AUTO[] = "b6351f02-1727-48cc-a950-9cf0312be9b5";
inline constexpr char UOS_FREE_DEEPSEEK_V3_2[] = "fea32769-a645-4e2d-aa01-684d5ef74e60";
inline constexpr char UOS_FREE_GLM_4_7[] = "da72278f-dfa3-4b5d-943f-be2b220a7380";
inline constexpr char UOS_FREE_DEEPSEEK_V4_PRO[] = "aa3c5b09-f868-4eee-a8c1-672bdec5c24e";
inline constexpr char UOS_FREE_DEEPSEEK_V4_FLASH[] = "e43950bc-5d63-44bd-aa7a-e84fac9256b3";
inline constexpr char UOS_FREE_DOOUBAO_SEED_1_8[] = "79b85080-ffe4-4c66-a64c-99f7c3416b7e";
inline constexpr char UOS_FREE_ONLINE_SEARCH[] = "deb126b9-6989-44a4-84a5-db59af2b94ff";
inline constexpr char OPENAI_GPT_3_5[] = "411d249b-c6b5-4350-ac42-6eedb601cc7e";
inline constexpr char OPENAI_GPT_4[] = "6668f539-51bd-41b6-8790-d516cd0443dd";
inline constexpr char DEEPSEEK_DEEPSEEK_V3_2[] = "9fad318a-cf48-42a7-a709-f536fa68c684";
inline constexpr char DEEPSEEK_DEEPSEEK_V4_FLASH[] = "8c672a06-4f31-4167-93de-534f1749754f";
inline constexpr char DEEPSEEK_DEEPSEEK_V4_PRO[] = "414ecda1-6917-47b9-a1ca-a0c87905965e";
inline constexpr char OPENAI_GPT_4_1[] = "7b1704dd-8ff0-4d4c-b2b6-b4b1c0716af7";
inline constexpr char OPENAI_GPT_O_1[] = "9944ddf1-c1b6-4ffa-b993-08ec0f8e18c8";
inline constexpr char OPENAI_GPT_5_3[] = "cf5d7a41-cc50-4d36-bdc4-8798518f3596";
inline constexpr char MINIMAX_MINIMAX_M2_5[] = "8cb8247c-9802-473a-a498-4134e25091bd";
inline constexpr char MINIMAX_MINIMAX_M2_7[] = "62802408-9b8a-411c-a7b8-112f39d1ecf7";
inline constexpr char MINIMAX_MINIMAX_M2_7_HIGHSPEED[] = "9b438bb7-7234-4ba0-a2dc-be2b44217e74";
inline constexpr char MOONSHOT_KIMI_2[] = "5c859e8e-b944-4fca-a627-6a4cdd17cdbf";
inline constexpr char MOONSHOT_KIMI_2_5[] = "e5663db0-6fb8-4d3c-8931-e9000e2f7abe";
inline constexpr char MOONSHOT_KIMI_2_6[] = "48684b10-b1f8-49fe-acf5-3b29b2f38df3";
inline constexpr char VOLCENGINE_DOOUBAO_SEED_2_0[] = "ef06a9c5-72f8-493b-bf6d-41986c9b72f6";
inline constexpr char VOLCENGINE_DOOUBAO_SEED_2_0_CODE[] = "fc59a9d7-fe33-4099-acea-f78d37387963";
inline constexpr char BIGMODEL_GLM_4_7[] = "61576493-c7e4-4596-b16e-d8cbbb706753";
inline constexpr char BIGMODEL_GLM_5[] = "217f40bc-76e7-4968-92df-b726f1ebc195";
inline constexpr char BIGMODEL_GLM_5_TURBO[] = "f9e7e099-5db2-4b4c-a4d8-d73df5c1712d";
inline constexpr char BIGMODEL_GLM_5V_TURBO[] = "8c055d36-3a74-42e0-ac4b-3527f5923af3";
inline constexpr char BIGMODEL_GLM_5_1[] = "dbc3b6f2-1e19-40f0-9ee2-315729483cce";
inline constexpr char BAILIAN_QWEN_3[] = "0b9d57f3-6a70-4146-b409-d47dbb913daa";
inline constexpr char BAILIAN_QWEN_3_5[] = "f09693ee-fff6-4bef-8350-e3922bf0af20";
inline constexpr char BAILIAN_QWEN_3_6_PLUS[] = "4322c1f0-9870-4d2d-85fd-6d12f6c4a2dc";
inline constexpr char BAILIAN_QWEN_3_6_FLASH[] = "0e247572-c341-4fe0-9f5a-48ceebf49564";
inline constexpr char BAILIAN_QWEN_3_6_MAX_PREVIEW[] = "822a5403-d985-4155-aac7-e4a5b209cc7a";

/**
 * @brief BuiltinProvider类
 * 内置模型服务提供商信息管理类（单例模式）
 * 提供预定义的模型服务提供商及其支持的模型列表
 */
class BuiltinProvider : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例
     * @return BuiltinProvider 单例实例
     */
    static BuiltinProvider *instance();

    /**
     * @brief 内置服务提供商信息结构
     */
    struct ProviderInfo {
        QString id;           // 服务商ID
        QString name;         // 服务商名称
        QString icon;         // 服务商图标路径
        QList<QString> models; // 支持的模型列表
    };

    /**
     * @brief 获取所有内置服务提供商信息
     * @return 服务提供商ID到ProviderInfo的映射
     */
    ProviderInfo queryProvider(const QString &id) const;

    /**
     * @brief 检查模型是否支持
     * @param id 模型ID
     * @return true 如果模型支持
     */
    bool isModelSupported(const QString &id) const;

    /**
     * @brief 检查服务商是否支持
     * @param id 服务商ID
     * @return true 如果服务商支持
     */
    bool isProviderSupported(const QString &id) const;

    /**
     * @brief 获取模型信息
     * @param id 模型ID
     * @return 模型信息
     */
    ModelInfo getModelInfo(const QString &id) const;

    /**
     * @brief 刷新内置服务商数据
     */
    void refreshProviders();

    static ProviderAccount xfInline();
private:
    explicit BuiltinProvider(QObject *parent = nullptr);

    // 初始化内置服务商数据
    void initializeProviders();
    void initializeExternalProviders();

    // 内置服务商数据缓存
    QMap<QString, ProviderInfo> m_providers;
    QMap<QString, ModelInfo> m_models;
};

} // namespace uos_ai

#endif // BUILTINPROVIDER_H
