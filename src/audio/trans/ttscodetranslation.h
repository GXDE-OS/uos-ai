#ifndef TTSCODETRANSLATION_H
#define TTSCODETRANSLATION_H

#include <QString>

class TtsCodeTranslation
{
public:
    static QString serverCodeTranslation(int code, const QString &message);

private:
    static QMap<int, QString> errorMessages();
};

#endif // TTSCODETRANSLATION_H
