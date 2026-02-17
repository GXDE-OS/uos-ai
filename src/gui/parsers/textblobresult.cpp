// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "textblobresult.h"

UOSAI_BEGIN_NAMESPACE

TextBlobResult::TextBlobResult(const QString &content)
    : m_content(content)
    , m_status(0)
    , m_errorString()
{
}

TextBlobResult::~TextBlobResult()
{
}

int TextBlobResult::status() const
{
    return m_status;
}

int TextBlobResult::totalSize() const
{
    return m_content.size();
}

QString TextBlobResult::getAllText() const
{
    return m_content;
}

QString TextBlobResult::getTextRange(int start, int length)
{
    if (start < 0 || start >= m_content.size()) {
        return QString();
    }
    
    if (length < 0) {
        return m_content.mid(start);
    }
    
    return m_content.mid(start, length);
}

QString TextBlobResult::errorString() const
{
    return m_errorString;
}

void TextBlobResult::setContent(const QString &content)
{
    m_content = content;
}

void TextBlobResult::setStatus(int status)
{
    m_status = status;
}

void TextBlobResult::setErrorString(const QString &error)
{
    m_errorString = error;
}

UOSAI_END_NAMESPACE
