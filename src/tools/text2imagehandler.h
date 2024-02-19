#ifndef TEXT2IMAGEHANDLER_H
#define TEXT2IMAGEHANDLER_H

#include <QJsonObject>

class Text2ImageHandler
{
public:
    /**
     * @brief imagePrompt
     * @param response
     * @param conversation
     * @return
     */
    static QString imagePrompt(const QJsonObject &response, const QString &conversation);
};

#endif // TEXT2IMAGEHANDLER_H
