#include "modeltool.h"
#include "global_key_define.h"

#include <QJsonObject>
#include <QJsonArray>

using namespace uos_ai;

ModelTool uos_ai::ModelTool::fromOai(const QJsonObject &toolObj)
{
    ModelTool modelTool;

    // 解析OpenAI格式的工具定义
    modelTool.name = toolObj.value(STR_KEY_NAME).toString();
    modelTool.description = toolObj.value(STR_KEY_DESCRIPTION).toString();

    // 解析parameters
    if (toolObj.contains(STR_KEY_PARAMETERS) && toolObj[STR_KEY_PARAMETERS].isObject()) {
        QJsonObject paramsObj = toolObj[STR_KEY_PARAMETERS].toObject();

        // 解析required字段
        if (paramsObj.contains(STR_KEY_REQUIRED) && paramsObj[STR_KEY_REQUIRED].isArray()) {
            QJsonArray requiredArray = paramsObj[STR_KEY_REQUIRED].toArray();
            for (const QJsonValue &requiredValue : requiredArray) {
                modelTool.required.append(requiredValue.toString());
            }
        }

        // 解析properties
        if (paramsObj.contains(STR_KEY_PROPERTIES) && paramsObj[STR_KEY_PROPERTIES].isObject()) {
            QJsonObject propertiesObj = paramsObj[STR_KEY_PROPERTIES].toObject();
            for (auto it = propertiesObj.begin(); it != propertiesObj.end(); ++it) {
                if (it.value().isObject()) {
                    QJsonObject propObj = it.value().toObject();
                    ModelToolProperty prop;
                    prop.name = it.key();
                    prop.type = propObj.value(STR_KEY_TYPE).toString();
                    prop.description = propObj.value(STR_KEY_DESCRIPTION).toString();
                    if (propObj.contains(STR_KEY_ENUM) && propObj[STR_KEY_ENUM].isArray()) {
                        QJsonArray enumArray = propObj[STR_KEY_ENUM].toArray();
                        for (const QJsonValue &enumValue : enumArray) {
                            prop.enums.append(enumValue.toString());
                        }
                    }
                    modelTool.properties.append(prop);
                }
            }
        }
    }

    return modelTool;
}
