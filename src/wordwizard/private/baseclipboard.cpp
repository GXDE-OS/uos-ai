#include "baseclipboard.h"

#ifdef COMPILE_ON_V25
#include "treelandclipboard.h"
#endif
#ifdef COMPILE_ON_V20
#include "waylandclipboard.h"
#endif
#include "esystemcontext.h"
#include "xclipboard.h"


UOSAI_USE_NAMESPACE

BaseClipboard::BaseClipboard(QObject *parent) : QObject(parent)
{

}

BaseClipboard *BaseClipboard::instance()
{
#ifdef COMPILE_ON_V25
    if (ESystemContext::isWayland()) {
        static TreelandClipboard instance;
        return &instance;
    }
#endif
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        static WaylandClipboard instance;
        return &instance;
    }
#endif
    static XClipboard instance;
    return &instance;
}

BaseClipboard *BaseClipboard::ttsInstance()
{
#ifdef COMPILE_ON_V25
    if (ESystemContext::isWayland()) {
        static TreelandClipboard instance;
        return &instance;
    }
#endif
#ifdef COMPILE_ON_V20
    if (ESystemContext::isWayland()) {
        static WaylandClipboard instance;
        return &instance;
    }
#endif
    static XClipboard instance;
    return &instance;
}

