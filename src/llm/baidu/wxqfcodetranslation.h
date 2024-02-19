#ifndef WXQFCODETRANSLATION_H
#define WXQFCODETRANSLATION_H

#include <QString>

class WXQFCodeTranslation
{
public:
    static QString serverCodeTranlation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // WXQFCODETRANSLATION_H
