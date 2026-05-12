#ifndef IATCODETRANSLATION_H
#define IATCODETRANSLATION_H

#include <QString>
namespace uos_ai {

class IatCodeTranslation
{
public:
    static QString serverCodeTranslation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};
} // namespace uos_ai
#endif // IATCODETRANSLATION_H
