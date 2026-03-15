#ifndef RESEARCHTOOLS_H
#define RESEARCHTOOLS_H

#include <QString>
#include <QJsonObject>

namespace uos_ai {

class ResearchTools
{
public:
    static QString readDocument(const QString &documentPath);
    static bool md2Word(const QString &md, const QString &wordPath);
    static bool md2Pdf(const QString &md, const QString &pdfPath);
    static bool checkAgentInstalled();

    static QString getFileIconData(const QString &docPath); // base64
    static void outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown);
};

} // namespace uos_ai

#endif // RESEARCHTOOLS_H
