#ifndef WEBSCRAPER_H
#define WEBSCRAPER_H

#include "agent/mcpclient.h"

namespace uos_ai {

class WebScraper
{
public:
    WebScraper();

    /**
     * @brief Scrapes the content of a given URL.
     * @param url The URL of the webpage to scrape.
     * @return The scraped content as a string, or an empty string on failure.
     */
    QString scrape(const QString &urlStr);

private:
    McpClient m_mcpClient;
};

} // namespace uos_ai

#endif // WEBSCRAPER_H
