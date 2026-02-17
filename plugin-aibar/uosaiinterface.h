#ifndef UOSAIINTERFACE_H
#define UOSAIINTERFACE_H

#include <QObject>

namespace uos_ai {

class UosAiInterface : public QObject
{
    Q_OBJECT
public:
    explicit UosAiInterface(QObject *parent = nullptr);
    void sendFile(const QString &file) const;
    void summaryFile(const QString &file) const;
    void translateFile(const QString &file) const;
    void correctFile(const QString &file) const;
    void addToKnowledgeBase(const QString &file) const;
signals:
protected:
    void inputPrompt(const QString &question, const QString &file) const;
};

}

#endif // UOSAIINTERFACE_H
