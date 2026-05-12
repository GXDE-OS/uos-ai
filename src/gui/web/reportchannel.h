#ifndef REPORTCHANNEL_H
#define REPORTCHANNEL_H

#include <QObject>
#include <QVariantMap>

namespace uos_ai {

class ReportChannel : public QObject
{
    Q_OBJECT
public:
    explicit ReportChannel(QObject *parent = nullptr);
    ~ReportChannel() override;

public slots:
    /**
     * @brief Write a report event from frontend
     * @param jsonData JSON string containing:
     *   - "type": int - the report point type enum value
     *   - "params": QVariant (optional) - constructor parameters
     *     - For QString: pass a string directly
     *     - For int: pass a number
     *
     * Example JSON:
     * {"type": 19, "params": {"assistant_type": "general"}}
     * {"type": 17, "params": 42}
     * {"type": 1}  // no params needed
     */
    void writeReportEvent(const QString &jsonData);

private:
    /**
     * @brief Create a report point and return its assembled data
     * @param type The report point type enum value
     * @param params The constructor parameters (if any)
     * @return QVariantMap containing the assembled data, empty if error
     */
    QVariantMap createReportPoint(int type, const QVariant &params);

    // Report type enum matching frontend
    enum ReportType {
        // No-argument types
        AiBarPoint = 1,
        WriterPoint = 2,
        ChatwindowPoint = 3,
        ScreenShotClickedPoint = 4,
        DigitalChatPoint = 5,
        PrivateChatClickedPoint = 6,
        ChatwindowStartPoint = 7,
        FollowalongPoint = 8,
        // String parameter types
        WriterFunctionPoint = 9,
        MCPChatPoint = 10,
        AssistantChatTypePoint = 11,
        KnowledgeFunctionPoint = 12,
        PrivateChatPoint = 13,
        FunctioncallPoint = 14,
        KnowledgeFileTypePoint = 15,
        // Integer parameter types
        FollowFunctionPoint = 16,
        KnowledgeFileNumberPoint = 17,
        // Special types with object parameters
        ModelPoint = 18,
        AssistantChatPoint = 19,
    };
};

} // namespace uos_ai

#endif // REPORTCHANNEL_H
