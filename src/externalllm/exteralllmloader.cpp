// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exteralllmloader_p.h"

#include <QDirIterator>
#include <QSettings>
#include <QStandardPaths>

#include <QDebug>

using namespace uos_ai;

ExteralLLMLoaderPrivate::ExteralLLMLoaderPrivate(ExteralLLMLoader *parent) : q(parent)
{

}


ExteralLLMLoader::ExteralLLMLoader(QObject *parent)
    : QObject(parent)
    , d(new ExteralLLMLoaderPrivate(this))
{
    QStringList paths{EXTERNAL_LLM_DIR};
    d->loadPaths = paths;
}

ExteralLLMLoader::~ExteralLLMLoader()
{
    delete d;
    d = nullptr;
}

void ExteralLLMLoader::setPaths(const QStringList &paths)
{
    d->loadPaths = paths;
}

void ExteralLLMLoader::readPlugins()
{
    d->llmPlugins.clear();
    d->loaders.clear();

    for (const QString &path : d->loadPaths) {
        QDirIterator dirItera(path, { "*.so" },
                              QDir::Filter::Files,
                              QDirIterator::IteratorFlag::NoIteratorFlags);
        while (dirItera.hasNext()) {
            dirItera.next();
             const QString &fileName { dirItera.path() + "/" + dirItera.fileName() };
            QSharedPointer<QPluginLoader> loader(new QPluginLoader);
            loader->setFileName(fileName);

            QJsonObject &&metaJson = loader->metaData();
            QString &&iid = metaJson.value("IID").toString();

            if (iid == LLMPlugin_Meta_IID) {
                if (!loader->load()) {
                    qWarning() << loader->errorString();
                    continue;
                }

                LLMPlugin *llm = dynamic_cast<LLMPlugin *>(loader->instance());
                if (!llm) {
                    qCritical() << loader->fileName() << "has no LLMPlugin.";
                    loader->unload();
                    continue;
                }

                d->llmPlugins.append(QSharedPointer<LLMPlugin>(llm));
                d->loaders.append(loader);
            }
        }
    }
}

QList<QSharedPointer<LLMPlugin> > ExteralLLMLoader::plugins() const
{
    return d->llmPlugins;
}
