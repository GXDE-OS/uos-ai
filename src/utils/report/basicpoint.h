#ifndef BASICPOINT_H
#define BASICPOINT_H

#include "uosai_global.h"

#include <QPair>
#include <QVariantMap>

UOSAI_BEGIN_NAMESPACE

namespace report {

enum EventID {
    // Each event has its unique id defined by data platform except Default id.
    Default              = 0,  // default event id, which is only used to identify base data
    FOLLOWALONG          = 1001600001,
    FOLLOWALONG_FUNCTION = 1001600002,
    WRITER               = 1001600003,
    WRITER_FUNCTION      = 1001600004,
    AI_BAR               = 1001600005,
    CHATWINDOW           = 1001600006,
    FUNCTIONCALL         = 1001600007,
    ASSISTANT_CHAT       = 1001600008,
    PRIVATE_CHAT_CLICKED = 1001600009,
    PRIVATE_CHAT         = 1001600010,
    DIGITAL_CHAT         = 1001600011,
    SCREENSHOT_CLICKED   = 1001600012,
    ASSISTANT_CHAT_TYPE  = 1001600013,
    MCP_CHAT             = 1001600014,
    CHATWINDOW_START     = 1001600015,
    MODEL                = 1001600016,
    KNOWLEDGE_FUNCTION   = 1001600017,
    KNOWLEDGE_FILE_TYPE  = 1001600018,
    KNOWLEDGE_FILE_NUMBER= 1001600019,
};

class BasicPoint
{
public:
    explicit BasicPoint();
    virtual ~BasicPoint();

    virtual QVariantMap assemblingData() const;
    virtual void setAdditionalData(const QVariantMap &data);

protected:
    QPair<QString, EventID> m_eventId;
    QVariantMap m_additionalData;
    QString m_event;
};

}

UOSAI_END_NAMESPACE

#endif // BASICPOINT_H
