// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PARSERFACTORY_H
#define PARSERFACTORY_H

#include "uosai_global.h"
#include "abstractfileparser.h"

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QMutex>
#include <QVariantList>

#include <functional>

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Factory for creating and managing file parsers
 * 
 * Provides centralized management of file parsers with support for
 * automatic parser selection based on file type, MIME type, or extension.
 * Uses a factory pattern with registration system for extensibility.
 */
class ParserFactory : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Parser creator function type
     */
    using ParserCreator = std::function<AbstractFileParser*()>;
    
    /**
     * @brief Get singleton instance
     * @return ParserFactory instance
     */
    static ParserFactory* instance();
    
    void loadDefaultParser();

    /**
     * @brief Register a parser type
     * @param parserName Name of the parser
     * @param creator Function to create parser instances
     * @return true on success, false if already registered
     */
    bool registerParser(const QString &parserName, ParserCreator creator);
    
    /**
     * @brief Unregister a parser type
     * @param parserName Name of the parser to unregister
     * @return true on success, false if not found
     */
    bool unregisterParser(const QString &parserName);
    
    /**
     * @brief Create a parser instance by name
     * @param parserName Name of the parser to create
     * @return Parser instance or nullptr if not found
     */
    AbstractFileParser* createParser(const QString &parserName);
    
    /**
     * @brief Create a parser for the given file path
     * @param filePath Path to the file
     * @return Parser instance or nullptr if no suitable parser found
     */
    AbstractFileParser* createParserForFile(const QString &filePath);
    
    /**
     * @brief Create a parser for the given MIME type
     * @param mimeType MIME type
     * @return Parser instance or nullptr if no suitable parser found
     */
    AbstractFileParser* createParserForMimeType(const QString &mimeType);
    
    /**
     * @brief Create a parser for the given file extension
     * @param extension File extension (with or without leading dot)
     * @return Parser instance or nullptr if no suitable parser found
     */
    AbstractFileParser* createParserForExtension(const QString &extension);
    
    /**
     * @brief Get all registered parser names
     * @return List of parser names
     */
    QStringList getRegisteredParsers() const;
    
    /**
     * @brief Get parser information by name
     * @param parserName Name of the parser
     * @return Parser instance for querying info, or nullptr if not found
     */
    AbstractFileParser* getParserInfo(const QString &parserName) const;
    
    /**
     * @brief Get parsers that support a specific MIME type
     * @param mimeType MIME type to check
     * @return List of parser names that support the MIME type
     */
    QStringList getParsersForMimeType(const QString &mimeType) const;
    
    /**
     * @brief Get parsers that support a specific file extension
     * @param extension File extension to check
     * @return List of parser names that support the extension
     */
    QStringList getParsersForExtension(const QString &extension) const;
    
    /**
     * @brief Check if a parser is registered
     * @param parserName Name of the parser to check
     * @return true if registered, false otherwise
     */
    bool isParserRegistered(const QString &parserName) const;
    
    /**
     * @brief Get the best parser for a file
     * @param filePath Path to the file
     * @return Name of the best parser or empty string if none found
     */
    QString getBestParserForFile(const QString &filePath) const;
    
    /**
     * @brief Get the best parser for a MIME type
     * @param mimeType MIME type
     * @return Name of the best parser or empty string if none found
     */
    QString getBestParserForMimeType(const QString &mimeType) const;
    
    /**
     * @brief Get the best parser for a file extension
     * @param extension File extension
     * @return Name of the best parser or empty string if none found
     */
    QString getBestParserForExtension(const QString &extension) const;
    
    /**
     * @brief Clear all registered parsers
     */
    void clearParsers();
    
    /**
     * @brief Get factory statistics
     * @return Statistics as QVariantMap
     */
    QVariantMap getStatistics() const;

Q_SIGNALS:
    /**
     * @brief Emitted when a parser is registered
     * @param parserName Name of the registered parser
     */
    void parserRegistered(const QString &parserName);
    
    /**
     * @brief Emitted when a parser is unregistered
     * @param parserName Name of the unregistered parser
     */
    void parserUnregistered(const QString &parserName);

private:
    explicit ParserFactory(QObject *parent = nullptr);
    ~ParserFactory() = default;
    
    // Disable copy constructor and assignment operator
    ParserFactory(const ParserFactory&) = delete;
    ParserFactory& operator=(const ParserFactory&) = delete;
            
    QString findBestParser(const QString &extension, const QString &mimeType) const;
private:
    QHash<QString, ParserCreator> m_parsers;
    mutable QMutex m_mutex;
};

UOSAI_END_NAMESPACE

#endif // PARSERFACTORY_H
