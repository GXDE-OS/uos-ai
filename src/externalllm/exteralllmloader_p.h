// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERALLLMLOADER_P_H
#define EXTERALLLMLOADER_P_H

#include "exteralllmloader.h"

#include "llmplugin.h"
#include "llmmodel.h"

#include <QPluginLoader>

namespace uos_ai {

class ExteralLLMLoaderPrivate
{
public:
    explicit ExteralLLMLoaderPrivate(ExteralLLMLoader *parent);
public:
    QStringList loadPaths;
    QList<QSharedPointer<QPluginLoader>> loaders;
    QList<QSharedPointer<LLMPlugin>> llmPlugins;

private:
    ExteralLLMLoader *q;
};

}

#endif // EXTERALLLMLOADER_P_H
