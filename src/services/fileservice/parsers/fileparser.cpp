// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileparser.h"
#include "docparse.h"
#include "ocrparse.h"
#include "textparse.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSet>

Q_DECLARE_LOGGING_CATEGORY(logService)

using namespace uos_ai;

static const QSet<QString> &docExts()
{
    static const QSet<QString> s = {
        "pdf",
        "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "odt", "ods",  "odp", "wps",  "dps"
    };
    return s;
}

static const QSet<QString> &imgExts()
{
    static const QSet<QString> s = { "jpg", "jpeg", "png" };
    return s;
}

static const QSet<QString> &txtExts()
{
    static const QSet<QString> s = {
        "txt",  "md",
        "c",    "cpp",  "cxx",  "cc",   "c++",  "h",    "hpp",
        "java", "py",   "js",   "ts",   "jsx",  "tsx",  "vue",  "svelte",
        "kt",   "kts",  "swift","go",   "rs",   "php",  "rb",   "pl",   "pm",
        "lua",  "r",    "m",    "mm",   "dart", "scala","groovy","sc",
        "sh",   "zsh",  "bash", "ps1",  "bat",  "cmd",
        "sql",  "json", "xml",  "yml",  "yaml", "toml", "ini",  "env",
        "gradle","pom", "properties","cmake","pro","sln","csproj",
        "vbproj","vcxproj",
        "qml",  "qbs",  "pri",
        "html", "htm",  "css",  "scss", "sass", "less",
        "jsp",  "asp",  "aspx", "ejs",  "erb",  "pug",  "jade",
        "handlebars","hbs","mustache","twig",
        "cs",   "fs",   "fsx",  "fsi",  "fsl",  "vb",
        "hs",   "d",    "elm",  "clj",  "cljs", "cljc",
        "erl",  "hrl",  "ex",   "exs",
        "asm",  "s",    "v",    "vh",   "vhd",  "vhdl", "sv",   "svh",
        "gitignore","dockerfile","tf","ipynb","jl"
    };
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────

FileParser::Strategy FileParser::strategyFor(const QString &filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();

    if (docExts().contains(ext)) return Strategy::Doc;
    if (imgExts().contains(ext)) return Strategy::Ocr;
    if (txtExts().contains(ext)) return Strategy::Text;
    return Strategy::Unsupported;
}

QString FileParser::parse(const QString &filePath, QString *errorMsg)
{
    switch (strategyFor(filePath)) {
    case Strategy::Doc:
        return DocParse::parse(filePath, errorMsg);
    case Strategy::Text:
        return TextParse::parse(filePath, errorMsg);
    case Strategy::Ocr:
        return OcrParse::parse(filePath, errorMsg);
    default:
        if (errorMsg)
            *errorMsg = QCoreApplication::translate("FileParser", "Unsupported file format");
        return {};
    }
}

QString FileParser::parseBase64Images(const QStringList &base64Images)
{
    return OcrParse::parseBase64Images(base64Images);
}

const QStringList &FileParser::supportedSuffixes()
{
    static const QStringList suffixes = []() {
        QStringList list;
        const auto addSet = [&](const QSet<QString> &set) {
            for (const QString &ext : set)
                list << "*." + ext;
        };
        addSet(docExts());
        addSet(imgExts());
        addSet(txtExts());
        list.sort();
        return list;
    }();
    return suffixes;
}
