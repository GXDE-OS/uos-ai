#ifndef BROWSWENATIVEDBUSOBJECT_H
#define BROWSWENATIVEDBUSOBJECT_H

#include "appdbusobject.h"

class BrowsweNativeDbusObject : public AppDbusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.browsernative.app")

public:
    explicit BrowsweNativeDbusObject(const QString &appId);
    ~BrowsweNativeDbusObject();

public Q_SLOTS:
    /**
     * @brief Query detailed account information for the LLM model, including the KEY.
     * @return JsonObject
     */
    Q_SCRIPTABLE QString queryCurrentDetailLLMAccount();

    /**
     * @brief isFreeAccountValid
     * @param llmId
     * @return 0:Valid, 1:expired, 2:usagelimit, 9999:error
     */
    Q_SCRIPTABLE int isFreeAccountValid(const QString &llmId);

    /**
     * @brief increment account usage count
     * @param llmId
     */
    Q_SCRIPTABLE void incrementUsageCount(const QString &llmId);
};

#endif // BROWSWENATIVEDBUSOBJECT_H
