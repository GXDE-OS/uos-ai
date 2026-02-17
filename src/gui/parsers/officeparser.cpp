// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "officeparser.h"
#include "textblobresult.h"
#include "utils/util.h"

#include <QFile>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

#include <docparser.h>

UOSAI_BEGIN_NAMESPACE

OfficeParser::OfficeParser(QObject *parent)
    : AbstractFileParser(parent)
{
}

AbstractFileParser *OfficeParser::create()
{
    return new OfficeParser;
}

QString OfficeParser::name() const
{
    return parserName();
}

QString OfficeParser::description() const
{
    return "Extracts text content from Microsoft Office documents (Word, Excel, PowerPoint)";
}

QHash<QString, float> OfficeParser::supportedMimeTypes() const
{
    QHash<QString, float> mimeTypes;
    mimeTypes["application/vnd.openxmlformats-officedocument.wordprocessingml.document"] = 1.0f;  // .docx
    mimeTypes["application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"] = 1.0f;       // .xlsx
    mimeTypes["application/vnd.openxmlformats-officedocument.presentationml.presentation"] = 1.0f; // .pptx
    mimeTypes["application/msword"] = 0.9f;                                                       // .doc
    mimeTypes["application/vnd.ms-excel"] = 0.9f;                                                // .xls
    mimeTypes["application/vnd.ms-powerpoint"] = 0.9f;                                           // .ppt
    mimeTypes["application/vnd.oasis.opendocument.text"] = 0.8f;                                 // .odt
    mimeTypes["application/vnd.oasis.opendocument.spreadsheet"] = 0.8f;                          // .ods
    mimeTypes["application/vnd.oasis.opendocument.presentation"] = 0.8f;                         // .odp
    return mimeTypes;
}

QHash<QString, float> OfficeParser::supportedExtensions() const
{
    QHash<QString, float> extensions;
    extensions["docx"] = 1.0f;
    extensions["xlsx"] = 1.0f;
    extensions["pptx"] = 1.0f;
    extensions["doc"] = 0.9f;
    extensions["xls"] = 0.9f;
    extensions["ppt"] = 0.9f;
    extensions["odt"] = 0.8f;
    extensions["ods"] = 0.8f;
    extensions["odp"] = 0.8f;
    extensions["wps"] = 0.8f;
    extensions["dps"] = 0.8f;

    return extensions;
}

QSharedPointer<ParserResult> OfficeParser::parseFile(const QString &filePath, const QVariantMap &metadata)
{
    auto ret = QSharedPointer<TextBlobResult>::create();

    std::string stdStrContents = DocParser::convertFile(filePath.toStdString());
    QString contents = Util::textEncodingTransferUTF8(stdStrContents);

    if (!Util::isValidDocContent(stdStrContents)) {
        ret->setStatus(-1);
        ret->setErrorString("Invalid document content.");
    } else {
        ret->setContent(contents);
    }

    return qSharedPointerDynamicCast<ParserResult>(ret);
}

UOSAI_END_NAMESPACE
