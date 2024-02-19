#ifndef CHATFIXEDRESPONSE_H
#define CHATFIXEDRESPONSE_H

#include <QString>

class ChatFixedResponse
{
public:
    /**
     * @brief checkRequestText
     * @param text
     * @return
     */
    static QString checkRequestText(const QString &text);
};

#endif // CHATFIXEDRESPONSE_H
