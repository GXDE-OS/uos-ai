// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXTBLOBRESULT_H
#define TEXTBLOBRESULT_H

#include "uosai_global.h"
#include "parserresult.h"

#include <QString>

UOSAI_BEGIN_NAMESPACE

/**
 * @brief Concrete implementation of ParserResult that stores content in a QString
 * 
 * This class provides a simple implementation where all parsed content
 * is stored in a single QString variable for easy access and manipulation.
 */
class TextBlobResult : public ParserResult
{
public:
    /**
     * @brief Constructor
     * @param content The text content to store
     */
    explicit TextBlobResult(const QString &content = QString());
    
    /**
     * @brief Destructor
     */
    virtual ~TextBlobResult();
    
    /**
     * @brief Get the parsing status code
     * @return Status code indicating parsing result
     */
    virtual int status() const override;
    
    /**
     * @brief Get the total size of parsed content in characters
     * @return Number of characters in the parsed content
     */
    virtual int totalSize() const override;
    
    /**
     * @brief Get all parsed text content
     * @return Complete parsed text content
     */
    virtual QString getAllText() const override;
    
    /**
     * @brief Get text content within specified range
     * @param start Start position (0-based index)
     * @param length Number of characters to retrieve
     * @return Text content within the specified range
     */
    virtual QString getTextRange(int start = 0, int length = -1) override;
    
    /**
     * @brief Get error string if parsing failed
     * @return Error message string
     */
    virtual QString errorString() const override;
    
    /**
     * @brief Set the content
     * @param content The text content to store
     */
    void setContent(const QString &content);
    
    /**
     * @brief Set the status code
     * @param status The status code
     */
    void setStatus(int status);
    
    /**
     * @brief Set the error string
     * @param error The error message
     */
    void setErrorString(const QString &error);

private:
    QString m_content;      ///< The stored text content
    int m_status;          ///< Parsing status code
    QString m_errorString; ///< Error message if parsing failed
};

UOSAI_END_NAMESPACE

#endif // TEXTBLOBRESULT_H
