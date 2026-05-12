#include "webscraper.h"

#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

WebScraper::WebScraper()
{
    // Initialize the MCP client and ensure the server is running.
    if (!m_mcpClient.init()) {
        qCWarning(logResearch) << "Failed to initialize McpClient for WebScraper.";
    }
}

QString WebScraper::scrape(const QString &urlStr)
{
    bool isPdf = (urlStr.split('.').last() == "pdf"); // TODO: uos-mcp解析网页工具无法处理pdf
    QUrl url = urlStr;
    if (!url.isValid() || isPdf) {
        qCWarning(logResearch) << "WebScraper received an invalid URL:" << url.toString();
        return QString();
    }

    // Define the agent, tool, and parameters for the MCP server call.
    const QString agentName = "default-agent";
    const QString toolName = "uos-mcp.fetch_web_content";
    QJsonObject params;
    params["url"] = url.toString();

    qCInfo(logResearch) << "Calling MCP tool 'scrape' for URL:" << url.toString();

    // Call the tool via McpClient.
    QPair<int, QString> result = m_mcpClient.callTool(agentName, toolName, params);

    // Check for errors.
    if (result.first != 0) {
        qCWarning(logResearch) << "WebScraper failed to scrape URL:" << url.toString()
                           << "Error code:" << result.first
                           << "Error message:" << result.second;
        return QString(); // Return empty string on failure.
    }

    // Return the scraped content on success.
    return result.second;
}

} // namespace uos_ai
