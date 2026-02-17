#include <memory>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcitx/ime.h>
#include <fcitx-config/hotkey.h>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utils.h>
#include <fcitx/instance.h>
#include <fcitx/keys.h>
#include <fcitx/module.h>
#include <fcitx/context.h>
#include <fcitx/module/punc/fcitx-punc.h>
#include <string>
#include <libintl.h>

#include "config.h"
#include "eim.h"
#include "bus.h"
#include "common.h"

extern "C" {
    FCITX_DEFINE_PLUGIN(fcitx_uosai, module, FcitxIMClass2) = {
        FcitxUosAiCreate,
        FcitxUosAiDestroy,
        FcitxUosAiReloadConfig,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };
}

void* FcitxUosAiCreate(FcitxInstance* instance)
{
    FcitxLog(INFO, "UosAi: Creating UosAi addon instance");
    FcitxUosAiAddonInstance* uosaiaddon = (FcitxUosAiAddonInstance*) fcitx_utils_malloc0(sizeof(FcitxUosAiAddonInstance));
    uosaiaddon->owner = instance;

    uosaiaddon->bus = new FcitxUosAiBus(uosaiaddon);
    FcitxLog(INFO, "UosAi: UosAi addon instance created successfully");

    return uosaiaddon;
}

/**
 * @brief Destroy the input method while unload it.
 *
 * @return int
 **/
void FcitxUosAiDestroy(void* arg)
{
    FcitxLog(INFO, "UosAi: Destroying UosAi addon instance");
}

void FcitxUosAiReloadConfig(void* arg)
{
}

