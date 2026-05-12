#ifndef ASSISTANTINFO_H
#define ASSISTANTINFO_H

#include "global_key_define.h"
#include "global_define.h"

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantHash>

namespace uos_ai {

/**
 * Metadata structure for an Assistant.
 * Contains static information like ID, name, icon, etc.
 */
struct AssistantInfo {
    QString id;
    QString name;
    QString description;
    QString path;
    QString placeHolder = QObject::tr("Ask a question...");
    
    // Map of icons: imageType as key, iconName as value
    // Example: { "line": "uos-ai", "outline": "uos-ai-outline" }
    QVariantHash icons;
    
    // Optional: for storing extra metadata if needed
    QVariantHash properties;

    /**
     * Convert metadata to JSON object.
     * Used for providing assistant list to UI/Clients.
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj[STR_KEY_ID] = id;
        obj[STR_KEY_NAME] = name;
        obj[STR_KEY_DESCRIPTION] = description;
        
        // Convert QVariantHash to QJsonObject for JSON serialization
        QJsonObject iconsObj;
        for (auto it = icons.constBegin(); it != icons.constEnd(); ++it) {
            iconsObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        obj[STR_KEY_ICON] = iconsObj;
        
        obj[STR_KEY_PATH] = path;
        obj[STR_KEY_PLACE_HOLDER] = placeHolder;
        return obj;
    }

    /**
     * Check if the info is valid.
     */
    bool isValid() const {
        return !id.isEmpty();
    }
};

} // namespace uos_ai

#endif // ASSISTANTINFO_H
