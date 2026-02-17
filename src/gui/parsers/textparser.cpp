// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "textparser.h"
#include "textblobresult.h"

#include <QFile>
#include <QTextStream>
#ifndef COMPILE_ON_QT6
#include <QTextCodec>
#endif
#include <QFileInfo>

UOSAI_BEGIN_NAMESPACE

TextParser::TextParser(QObject *parent)
    : AbstractFileParser(parent)
{
}

AbstractFileParser *TextParser::create()
{
    return new TextParser;
}

QString TextParser::name() const
{
    return parserName();
}

QString TextParser::description() const
{
    return "Parses plain text files with automatic encoding detection";
}

QHash<QString, float> TextParser::supportedMimeTypes() const
{
    QHash<QString, float> mimeTypes;
    mimeTypes["text/plain"] = 1.0f;
    mimeTypes["text/txt"] = 0.9f;
    mimeTypes["text/x-csrc"] = 0.8f;
    mimeTypes["text/x-chdr"] = 0.8f;
    mimeTypes["text/x-c++src"] = 0.8f;
    mimeTypes["text/x-java"] = 0.8f;
    mimeTypes["text/x-go"] = 0.8f;
    mimeTypes["text/rust"] = 0.8f;
    mimeTypes["application/x-zerosize"] = 0.8f;
    mimeTypes["application/javascript"] = 0.8f;
    mimeTypes["application/x-zerosize"] = 0.8f;
    mimeTypes["application/x-ruby"] = 0.8f;
    mimeTypes["application/x-php"] = 0.8f;
    mimeTypes["application/x-perl"] = 0.8f;
    mimeTypes["application/x-empty"] = 0.1f;
    return mimeTypes;
}

QHash<QString, float> TextParser::supportedExtensions() const
{
    const QStringList TEXT_EXTS = {
        "txt", "md", "c", "cpp", "cc", "cxx", "h", "hpp", "java", "py", "js", "ts",
        "jsx", "tsx", "vue", "svelte", "kt", "kts", "swift", "go", "rs", "php",
        "rb", "pl", "pm", "lua", "r", "m", "mm", "dart", "scala", "groovy", "sh",
        "zsh", "bash", "ps1", "bat", "cmd", "sql", "json", "xml", "yml", "yaml",
        "toml", "ini", "env", "gradle", "properties", "prop", "cmake", "pro",
        "qml", "qbs", "pri", "fsl", "fsi", "fs", "fsx", "hs", "d", "elm", "clj",
        "cljs", "cljc", "erl", "hrl", "ex", "exs", "v", "vh", "vhd", "vhdl",
        "sv", "svh", "asm", "s", "cs", "fs", "vb", "ts", "tsx", "jsx", "vue",
        "html", "htm", "css", "scss", "sass", "less", "jsp", "asp", "aspx",
        "ejs", "erb", "pug", "jade", "handlebars", "hbs", "mustache", "twig",
        "gitignore", "dockerfile", "tf", "ipynb"
    };

    QHash<QString, float> extensions;
    for (const QString &ext : TEXT_EXTS) {
        extensions[ext] = 1.0f;
    }

    return extensions;
}

QSharedPointer<ParserResult> TextParser::parseFile(const QString &filePath, const QVariantMap &metadata)
{
    auto result = QSharedPointer<TextBlobResult>::create();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QString errorMsg = QString("Failed to open file: %1").arg(file.errorString());
        result->setErrorString(errorMsg);
        result->setStatus(-1);
        return qSharedPointerDynamicCast<ParserResult>(result);
    }
    
    QByteArray data = file.readAll();

    if (looksBinary(data)) {
        result->setContent(QString());
        return qSharedPointerDynamicCast<ParserResult>(result);
    }

    // todo text decode
    QString content = QString::fromUtf8(data);
    file.close();
    
    result->setContent(content);
    return qSharedPointerDynamicCast<ParserResult>(result);
}

bool TextParser::looksBinary(const QByteArray &head)
{
    // UTF-8/ASCII 控制字符中，除 \t\n\r 外出现 \0 或 0x01-0x08,0x0B-0x0C,0x0E-0x1F 即可判为二进制
    const char *p = head.constData();
    int limit = qMin(4096, head.size());
    for (int i = 0; i < limit; ++i) {
        unsigned char c = static_cast<unsigned char>(p[i]);
        if (c == '\0' || (c < 0x20 && c != '\t' && c != '\n' && c != '\r'))
            return true;
    }
    return false;
}

UOSAI_END_NAMESPACE
