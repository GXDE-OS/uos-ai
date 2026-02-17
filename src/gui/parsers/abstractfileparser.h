// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ABSTRACTFILEPARSER_H
#define ABSTRACTFILEPARSER_H

#include "uosai_global.h"
#include "parserresult.h"

#include <QObject>
#include <QHash>
#include <QVariantMap>

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Abstract base class for file parsers
 * 
 * Provides the interface that all file parsers must implement.
 * Each parser is responsible for extracting text content from
 * specific file types.
 */
class AbstractFileParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit AbstractFileParser(QObject *parent = nullptr);
    
    /**
     * @brief Get parser name (unique identifier)
     * @return Parser name
     */
    virtual QString name() const = 0;
    
    /**
     * @brief Get parser description
     * @return Human-readable description
     */
    virtual QString description() const = 0;
    
    /**
     * @brief Get supported MIME types (supports regex patterns)
     * @return Hash of supported MIME types with their priority weights
     */
    virtual QHash<QString, float> supportedMimeTypes() const = 0;
    
    /**
     * @brief Get supported file extensions (supports regex patterns, no leading dot required)
     * @return Hash of supported file extensions with their priority weights
     */
    virtual QHash<QString, float> supportedExtensions() const = 0;
        
    /**
     * @brief Check if this parser can handle the given file
     * @param filePath Path to the file
     * @return true if this parser can handle the file, false otherwise
     */
    virtual bool canParse(const QString &filePath) const;
    
    /**
     * @brief Check if this parser can handle the given MIME type
     * @param mimeType MIME type to check
     * @return true if this parser supports the MIME type, false otherwise
     */
    virtual bool canParseMimeType(const QString &mimeType) const;
    
    /**
     * @brief Check if this parser can handle the given file extension
     * @param extension File extension (no leading dot required, supports regex matching)
     * @return true if this parser supports the extension, false otherwise
     */
    virtual bool canParseExtension(const QString &extension) const;
    
    /**
     * @brief Parse a file and extract text content
     * @param filePath Path to the file to parse
     * @param metadata Optional metadata to pass to the parser
     * @return QSharedPointer<ParserResult> containing parsed content and status information
     */
    virtual QSharedPointer<ParserResult> parseFile(const QString &filePath, const QVariantMap &metadata = QVariantMap()) = 0;
           
    /**
     * @brief Check if the parser is available (dependencies met)
     * @return true if the parser can be used, false otherwise
     */
    virtual bool isAvailable() const { return true; }
    
    /**
     * @brief Get parser priority for a specific file type
     * @param extension File extension
     * @param mimeType MIME type
     * @return Priority score (higher is better)
     */
    virtual float getPriority(const QString &extension = QString(), const QString &mimeType = QString()) const;

    static float matchPattern(const QString &input, const QHash<QString, float> &patterns);

Q_SIGNALS:
    /**
     * @brief Emitted when parsing progress changes
     * @param progress Progress percentage (0-100)
     */
    void parsingProgress(int progress);
    
    /**
     * @brief Emitted when parsing is completed
     * @param success Whether parsing succeeded
     * @param content Parsed content (if successful)
     */
    void parsingCompleted(bool success);
};

UOSAI_END_NAMESPACE

#endif // ABSTRACTFILEPARSER_H
