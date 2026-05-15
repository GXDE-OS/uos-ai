#include "deepseekmessageprotocol.h"
#include "global_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

DeepSeekMessageProtocol::DeepSeekMessageProtocol() : OaiMessageProtocol()
{
}

DeepSeekMessageProtocol::~DeepSeekMessageProtocol()
{
}

QJsonArray DeepSeekMessageProtocol::messages() const
{
    if (!m_reasoning)
        return OaiMessageProtocol::messages();

    int lastUserIndex = -1;
    for (int i = 0; i < m_messages.size(); ++i) {
        QJsonObject msgObj = m_messages[i].toObject();
        if (msgObj[STR_KEY_ROLE].toString() == STR_KEY_USER) {
            lastUserIndex = i;
        }
    }

    QJsonArray result;
    for (int i = 0; i < m_messages.size(); ++i) {
        QJsonObject msgObj = m_messages[i].toObject();
        if (i < lastUserIndex) {
            msgObj.remove(STR_KEY_REASONING_CONTENT);
        }
        result.append(msgObj);
    }

    return result;
}
