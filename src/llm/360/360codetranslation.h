#ifndef CODETRANSLATION360_H
#define CODETRANSLATION360_H

#include <QString>

class CodeTranslation360
{
public:
    static QString serverCodeTranlation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // CODETRANSLATION360_H
