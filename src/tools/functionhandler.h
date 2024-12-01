#ifndef FUNCTIONHANDLER_H
#define FUNCTIONHANDLER_H

#include <QJsonObject>

class FunctionHandler
{
public:
    /**
     * @brief queryFunctions
     * @return
     */
    static QJsonObject queryAppFunctions(bool &notYetQueried);

    /**
     * @brief functions
     * @return
     */
    static QJsonArray functions(const QJsonObject &appFunctions);

    /**
     * @brief functionPluginPath
     * @return
     */
    static QString functionPluginPath();

    /**
     * @brief functionCall
     * @param fun
     * @return
     */
    static QJsonArray functionCall(const QJsonObject &response, const QString &conversation, QString *directReply = nullptr);

    /**
     * @brief chatAction
     * @param response
     * @return
     */
    static int chatAction(const QJsonObject &response);

private:
    /**
     * @brief functionProcess
     * @param appFunctions
     * @param fun
     * @return
     */
    static QJsonObject functionProcess(const QJsonObject &appFunctions, QJsonObject fun, QString *directReply);
};

#endif // FUNCTIONHANDLER_H
