#ifndef AIWRITER_H
#define AIWRITER_H

#include "abstractassistant.h"
#include "agent/research/writingworkspace.h"

namespace uos_ai {

class AIWriter : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit AIWriter(QObject *parent = nullptr);
    ~AIWriter() override;

    void cancel() override;

    /** 返回最近编辑的写作文档列表（JSON 字符串），按 updatedAt 降序，最多 maxCount 条 */
    static QString getRecentDocs(int maxCount = 20);

    /** 返回写作模板列表（JSON 字符串），根据系统语言加载对应语言包，icon 字段已解析为 qrc:// 路径 */
    static QString getWritingTemplates();

Q_SIGNALS:
    void requestCancel();
protected:
    QVariantHash run() override;

private:
    void ensureWorkspace();

    WritingWorkspace m_workspace;
};

} // namespace uos_ai

#endif // AIWRITER_H
