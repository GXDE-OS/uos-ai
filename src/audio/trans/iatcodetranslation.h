#ifndef IATCODETRANSLATION_H
#define IATCODETRANSLATION_H

#include <QString>

class IatCodeTranslation
{
public:
    static QString serverCodeTranslation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // IATCODETRANSLATION_H
