#ifndef SERVICECONFIGCHANNEL_H
#define SERVICECONFIGCHANNEL_H

#include <QObject>
#include <QJsonObject>
#include <QString>

namespace uos_ai {

/**
 * @brief ServiceConfigChannel for service-style business configuration management.
 *
 * Boundary:
 * - Owns configuration-like business domains similar to MCP service management.
 * - Suitable for persistent service configuration, enable/disable state, runtime readiness,
 *   synchronization, and add/edit/delete operations around external or built-in services.
 * - Prefer explicit domain methods; do not turn this channel into a generic key-value config bucket.
 * - Not suitable for system resources or UI state such as theme, icon, translation, or window state.
 * - Not suitable for session events, conversation content, or file transfer workflows.
 *
 * Examples of suitable domains:
 * - MCP service management
 * - Future provider / connector / third-party service configuration
 */
class ServiceConfigChannel : public QObject
{
    Q_OBJECT
public:
    explicit ServiceConfigChannel(QObject *parent = nullptr);
    ~ServiceConfigChannel() override;
signals:
    void knowledgeBaseChanged(bool);
    void embeddingPluginsChanged(bool);
    void mcpPluginChanged(bool);
public slots:
    QJsonObject getMcpServices();
    QJsonObject saveMcpService(const QString &jsonConfig,
                               const QString &description,
                               const QString &editingServiceId);
    QJsonObject deleteMcpService(const QString &serviceId);
    QJsonObject setMcpServiceEnabled(const QString &serviceId, bool enabled);
    bool isMcpRuntimeReady();
    bool getMcpThirdPartyAgreement();
    void setMcpThirdPartyAgreement(bool agreed);

    void installApp(const QString &app);

    bool checkKnowledgeBase();
    bool checkEmbeddingPlugins();
    bool checkDocumentConversionCapability();
};

} // namespace uos_ai

#endif // SERVICECONFIGCHANNEL_H
