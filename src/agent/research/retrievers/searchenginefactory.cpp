#include "searchenginefactory.h"
#include "serper.h"
#include "baidusearch.h"
#include "volcanoengine.h"

namespace uos_ai {

QSharedPointer<SearchEngine> SearchEngineFactory::create(EngineType type)
{
    switch (type) {
    case EngineType::Serper:
        return QSharedPointer<Serper>::create();
    case EngineType::Baidu:
        return QSharedPointer<BaiduSearch>::create();
    case EngineType::Volcano:
        return QSharedPointer<VolcanoEngine>::create();
    default:
        return nullptr;
    }
}

} // namespace uos_ai
