// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERNALLLMLOADER_P_H
#define EXTERNALLLMLOADER_P_H

#include "externalllmloader.h"

#include "llmplugin.h"
#include "llmmodel.h"

#include <QPluginLoader>

namespace uos_ai {

class ExternalLLMLoaderPrivate
{
public:
    explicit ExternalLLMLoaderPrivate(ExternalLLMLoader *parent);
public:
    QStringList loadPaths;
    QList<QSharedPointer<QPluginLoader>> loaders;
    QList<QSharedPointer<LLMPlugin>> llmPlugins;

private:
    ExternalLLMLoader *q;
};

}

#endif // EXTERNALLLMLOADER_P_H
