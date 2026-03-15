#ifndef SEARCHENGINEFACTORY_H
#define SEARCHENGINEFACTORY_H

#include "searchengine.h"
#include <QSharedPointer>
#include <QString>

namespace uos_ai {

class SearchEngineFactory
{
public:
    enum class EngineType {
        Serper,
        Baidu
    };

    static QSharedPointer<SearchEngine> create(EngineType type);
};

} // namespace uos_ai

#endif // SEARCHENGINEFACTORY_H
