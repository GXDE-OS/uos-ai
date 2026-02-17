#ifndef EIM_H
#define EIM_H

#include <fcitx/ime.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/instance.h>
#include <fcitx/candidate.h>
#include <vector>
#include <string>
#include "bus.h"
#include "common.h"


struct FcitxUosAiConfig
{
    FcitxGenericConfig gconfig;
};


CONFIG_BINDING_DECLARE(FcitxUosAiConfig);
void* FcitxUosAiCreate(FcitxInstance* instance);
void FcitxUosAiDestroy(void* arg);
INPUT_RETURN_VALUE FcitxUosAiDoInput(void* arg, FcitxKeySym sym, unsigned int state);
boolean FcitxUosAiInit(void*);
void FcitxUosAiReloadConfig(void*);

typedef struct _FcitxUosAiAddonInstance {
    FcitxUosAiConfig config;
    FcitxInstance* owner;
    FcitxUosAiBus* bus;
} FcitxUosAiAddonInstance;

#endif
//
