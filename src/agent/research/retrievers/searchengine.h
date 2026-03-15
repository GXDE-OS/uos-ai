#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QObject>
#include <QString>
#include <QJsonArray>

namespace uos_ai {

class SearchEngine : public QObject
{
    Q_OBJECT
public:
    explicit SearchEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~SearchEngine() {}

    virtual QJsonArray search(const QString &query, int maxResults = 5) = 0;
};

} // namespace uos_ai

#endif // SEARCHENGINE_H
