#ifndef ICOMPONENTS_H
#define ICOMPONENTS_H

#include "global_key_define.h"

#include <QVariantHash>
#include <QObject>

namespace uos_ai {

inline QVariantHash makeBashApprove(const QString &requestId, const QString &title,
                                    const QString &command)
{
    QVariantHash data;
    data[STR_KEY_ID] = requestId;
    data["ic_type"] = "bash_approve";
    data[STR_KEY_TITLE] = title;
    data["command"] = command;
    data[STR_KEY_STATUS] = "pending";  // "pending":待批准；"rejected":拒绝；"approved":接受

    return data;
}

inline QVariantHash makeFileChangeApprove(const QString &requestId,
                                    const QVariantList &changes, int count)
{
    QVariantHash data;
    data[STR_KEY_ID] = requestId;
    data["ic_type"] = "file_change_approve";
    data[STR_KEY_TITLE] = QObject::tr("%0 file changes").arg(count);
    data["changes"] = changes;
    data[STR_KEY_STATUS] = "pending";  // "pending":待批准；"rejected":拒绝；"approved":接受
    return data;
}

}
#endif // ICOMPONENTS_H
