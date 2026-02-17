// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PARSERRESULT_H
#define PARSERRESULT_H

#include "uosai_global.h"

#include <QString>
#include <QStringList>
#include <QSharedPointer>

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Result container for file parsing operations
 * 
 * Provides structured access to parsed content with support for
 * text retrieval, size information, and range-based access.
 */
class ParserResult
{
public:
    /**
     * @brief Constructor
     */
    explicit ParserResult();
        
    /**
     * @brief Destructor
     */
    virtual ~ParserResult();
    
    /**
     * @brief Get the parsing status code
     * @return Status code indicating parsing result
     */
    virtual int status() const = 0;
        
    /**
     * @brief Get the total size of parsed content in characters
     * @return Number of characters in the parsed content
     */
    virtual int totalSize() const = 0;
    
    /**
     * @brief Get all parsed text content
     * @return Complete parsed text content
     */
    virtual QString getAllText() const = 0;
    
    /**
     * @brief Get text content within specified range
     * @param start Start position (0-based index)
     * @param length Number of characters to retrieve
     * @return Text content within the specified range
     */
    virtual QString getTextRange(int start = 0, int length = -1) = 0;

    /**
     * @brief Get the parsing error message
     * @return Error message
     */
    virtual QString errorString() const = 0;
};

UOSAI_END_NAMESPACE

#endif // PARSERRESULT_H
