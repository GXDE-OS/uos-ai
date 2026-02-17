// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "parserfactory.h"

#include "officeparser.h"
#include "pdfparser.h"
#include "textparser.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>

UOSAI_BEGIN_NAMESPACE

ParserFactory* ParserFactory::instance()
{
    static ParserFactory instance;
    return &instance;
}

void ParserFactory::loadDefaultParser()
{
    registerParser(OfficeParser::parserName(), &OfficeParser::create);
    registerParser(PdfParser::parserName(), &PdfParser::create);
    registerParser(TextParser::parserName(), &TextParser::create);
}

ParserFactory::ParserFactory(QObject *parent)
    : QObject(parent)
{
    loadDefaultParser();
}

bool ParserFactory::registerParser(const QString &parserName, ParserCreator creator)
{
    QMutexLocker locker(&m_mutex);
    
    if (parserName.isEmpty() || !creator) {
        return false;
    }
    
    if (m_parsers.contains(parserName)) {
        return false; // Already registered
    }
        
    m_parsers[parserName] = creator;
    
    emit parserRegistered(parserName);
    return true;
}

bool ParserFactory::unregisterParser(const QString &parserName)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_parsers.contains(parserName)) {
        return false;
    }
    
    m_parsers.remove(parserName);
    
    emit parserUnregistered(parserName);
    return true;
}

AbstractFileParser* ParserFactory::createParser(const QString &parserName)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_parsers.contains(parserName)) {
        return nullptr;
    }
    
    ParserCreator creator = m_parsers[parserName];
    return creator();
}

AbstractFileParser* ParserFactory::createParserForFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return nullptr;
    }
    
    QString extension = fileInfo.suffix();
    QString bestParser;
    if (!extension.isEmpty())
        bestParser = findBestParser(extension, "");

    if (bestParser.isEmpty()) {
        QString mimeType;
        QMimeDatabase mimeDb;
        QMimeType mime = mimeDb.mimeTypeForFile(filePath);
        if (mime.isValid())
            mimeType = mime.name();

        if (!mimeType.isEmpty())
            bestParser = findBestParser("", mimeType);
    }

    if (bestParser.isEmpty())
        return nullptr;

    return createParser(bestParser);
}

AbstractFileParser* ParserFactory::createParserForMimeType(const QString &mimeType)
{
    QString bestParser = getBestParserForMimeType(mimeType);
    if (bestParser.isEmpty())
        return nullptr;

    return createParser(bestParser);
}

AbstractFileParser* ParserFactory::createParserForExtension(const QString &extension)
{
    QString bestParser = getBestParserForExtension(extension);
    if (bestParser.isEmpty()) {
        return nullptr;
    }
    
    return createParser(bestParser);
}

QStringList ParserFactory::getRegisteredParsers() const
{
    QMutexLocker locker(&m_mutex);
    return m_parsers.keys();
}

AbstractFileParser* ParserFactory::getParserInfo(const QString &parserName) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_parsers.contains(parserName)) {
        return nullptr;
    }
    
    return m_parsers[parserName]();
}

QStringList ParserFactory::getParsersForMimeType(const QString &mimeType) const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList parsers;
    for (auto it = m_parsers.begin(); it != m_parsers.end(); ++it) {
        AbstractFileParser *parser = it.value()();
        if (parser && parser->canParseMimeType(mimeType)) {
            parsers.append(it.key());
        }
        delete parser;
    }
    
    return parsers;
}

QStringList ParserFactory::getParsersForExtension(const QString &extension) const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList parsers;
    for (auto it = m_parsers.begin(); it != m_parsers.end(); ++it) {
        AbstractFileParser *parser = it.value()();
        if (parser && parser->canParseExtension(extension)) {
            parsers.append(it.key());
        }
        delete parser;
    }
    
    return parsers;
}

bool ParserFactory::isParserRegistered(const QString &parserName) const
{
    QMutexLocker locker(&m_mutex);
    return m_parsers.contains(parserName);
}

QString ParserFactory::getBestParserForFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return QString();
    }
    
    QString extension = fileInfo.suffix();
    QString mimeType;
    
    // Get MIME type
    QMimeDatabase mimeDb;
    QMimeType mime = mimeDb.mimeTypeForFile(filePath);
    if (mime.isValid()) {
        mimeType = mime.name();
    }
    
    return findBestParser(mimeType, extension);
}

QString ParserFactory::getBestParserForMimeType(const QString &mimeType) const
{
    return findBestParser(mimeType, QString());
}

QString ParserFactory::getBestParserForExtension(const QString &extension) const
{
    return findBestParser(QString(), extension);
}

void ParserFactory::clearParsers()
{
    QMutexLocker locker(&m_mutex);
    m_parsers.clear();
}

QVariantMap ParserFactory::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QVariantMap stats;
    stats["totalParsers"] = m_parsers.size();
    
    QVariantList parserList;
    for (auto it = m_parsers.begin(); it != m_parsers.end(); ++it) {
        AbstractFileParser *parser = it.value()();
        QVariantMap parserInfo;
        parserInfo["name"] = it.key();
        if (parser) {
            parserInfo["mimeTypes"] = parser->supportedMimeTypes().size();
            parserInfo["extensions"] = parser->supportedExtensions().size();
        }
        parserList.append(parserInfo);
        delete parser;
    }
    stats["parsers"] = parserList;
    
    return stats;
}

QString ParserFactory::findBestParser(const QString &extension, const QString &mimeType) const
{
    QString bestParser;
    float bestScore = 0;
    
    for (auto it = m_parsers.begin(); it != m_parsers.end(); ++it) {
        AbstractFileParser *parser = it.value()();
        if (parser) {
            float score = parser->getPriority(extension, mimeType);
            if (score > bestScore) {
                bestScore = score;
                bestParser = it.key();
            }

            delete parser;
            parser = nullptr;
        }
    }
    
    return bestParser;
}

UOSAI_END_NAMESPACE
