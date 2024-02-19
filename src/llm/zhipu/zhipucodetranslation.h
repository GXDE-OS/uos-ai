#ifndef ZHIPUCODETRANSLATION_H
#define ZHIPUCODETRANSLATION_H

#include <QString>

class ZhiPuCodeTranslation
{
public:
    static QString serverCodeTranlation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // ZHIPUCODETRANSLATION_H
