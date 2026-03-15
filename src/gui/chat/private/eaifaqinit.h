#ifndef EAIFAQINIT_H
#define EAIFAQINIT_H

#include <QJsonArray>
#include <QObject>

class EAiFAQInit : public QObject
{
    Q_OBJECT
public:
    explicit EAiFAQInit(QObject *parent = nullptr);
    QJsonArray createTranslationFAQArray() const;
    QJsonArray createWritingFAQArray() const;
    QJsonArray createTextProcessingFAQArray() const;
    QJsonArray createWritingFunctionArray() const;
    QByteArray createWritingFunctionTemplate() const;
    QJsonArray createTextProcessingFunctionArray() const;
    QJsonArray assignRandomIcons(QJsonArray jsonArray, const QString iconName = "general") const;

private:
    // ... existing code ...
};

#endif // EAIFAQINIT_H
