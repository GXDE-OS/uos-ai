#include "searchenginefactory.h"
#include "serper.h"
#include "baidusearch.h"

namespace uos_ai {

QSharedPointer<SearchEngine> SearchEngineFactory::create(EngineType type)
{
    switch (type) {
    case EngineType::Serper:
        return QSharedPointer<Serper>::create();
    case EngineType::Baidu:
        return QSharedPointer<BaiduSearch>::create();
    default:
        return nullptr;
    }
}

} // namespace uos_ai
