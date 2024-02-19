#ifndef UTILS_H
#define UTILS_H

#include "llm.h"
#include "serverdefs.h"

#include <QString>
#include <QSharedPointer>

#define DBUS_SERVER             "com.deepin.copilot"
#define DBUS_SERVER_PATH        "/com/deepin/copilot"
#define DBUS_SERVER_INTERFACE   "com.deepin.copilot"

class AppDbusPathObject;
class LLMUtils
{
public:
    /**
     * @brief adjustDbusPath
     * @param path
     * @return
     */
    static QString adjustDbusPath(QString appId);

    /**
     * @brief queryAppId
     * @param pid
     * @return
     */
    static QString queryAppId(const uint &pid);

    /**
     * @brief getCopilot
     * @return
     */
    static QSharedPointer<LLM> getCopilot(const LLMServerProxy &serverproxy);

    /**
     * @brief systemEnvInfo
     * @return
     */
    static QString systemEnvInfo();
};

#endif // UTILS_H
