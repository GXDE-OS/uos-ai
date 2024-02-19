#ifndef XFCODETRANSLATION_H
#define XFCODETRANSLATION_H

#include <QString>

class XFCodeTranslation
{
public:
    static QString serverCodeTranslation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // XFCODETRANSLATION_H
