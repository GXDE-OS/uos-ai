#ifndef AGENTSTEP_H
#define AGENTSTEP_H

#include "conversation/messagenode.h"
#include "global_define.h"
#include "global_key_define.h"

#include <QVariantHash>

namespace uos_ai {

/**
 * 构造一条 AgentStep 渲染消息列表，供调用方 emit
 * 用法：emit messageReceived(makeAgentStep(title, NsRunning));
 *
 * @param title    步骤标题
 * @param status   步骤状态（默认 NsRunning）
 * @param content  步骤内容（可选）
 */
inline RenderMessageList makeAgentStep(const QString &title,
                                        NormalStatus status = NsRunning,
                                        const QString &content = {})
{
    QVariantHash data;
    data[STR_KEY_TITLE] = title;
    data[STR_KEY_STATUS] = static_cast<int>(status);
    data[STR_KEY_CONTENT] = content;

    RenderMessage rmsg;
    rmsg.type = ContentType::CntAgentStep;
    rmsg.data = data;
    return RenderMessageList{rmsg};
}

} // namespace uos_ai

#endif // AGENTSTEP_H
