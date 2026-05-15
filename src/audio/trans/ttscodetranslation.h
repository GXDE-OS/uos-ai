#ifndef TTSCODETRANSLATION_H
#define TTSCODETRANSLATION_H

#include <QString>
namespace uos_ai {

class TtsCodeTranslation
{
public:
    static QString serverCodeTranslation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};
} // namespace uos_ai
#endif // TTSCODETRANSLATION_H
