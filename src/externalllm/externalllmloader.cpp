// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externalllmloader_p.h"

#include <QDirIterator>
#include <QSettings>
#include <QStandardPaths>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)
using namespace uos_ai;

ExternalLLMLoaderPrivate::ExternalLLMLoaderPrivate(ExternalLLMLoader *parent) : q(parent)
{

}


ExternalLLMLoader::ExternalLLMLoader(QObject *parent)
    : QObject(parent)
    , d(new ExternalLLMLoaderPrivate(this))
{
    QStringList paths{EXTERNAL_LLM_DIR};
    d->loadPaths = paths;
}

ExternalLLMLoader::~ExternalLLMLoader()
{
    delete d;
    d = nullptr;
}

void ExternalLLMLoader::setPaths(const QStringList &paths)
{
    d->loadPaths = paths;
}

void ExternalLLMLoader::readPlugins()
{
    qCInfo(logExternalLLM) << "Starting to read external LLM plugins";
    d->llmPlugins.clear();
    d->loaders.clear();

    for (const QString &path : d->loadPaths) {
        qCDebug(logExternalLLM) << "Searching for plugins in path:" << path;
        QDirIterator dirItera(path, { "*.so" },
                              QDir::Filter::Files,
                              QDirIterator::IteratorFlag::NoIteratorFlags);
        while (dirItera.hasNext()) {
            dirItera.next();
            const QString &fileName { dirItera.path() + "/" + dirItera.fileName() };
            qCDebug(logExternalLLM) << "Found plugin file:" << fileName;
            QSharedPointer<QPluginLoader> loader(new QPluginLoader);
            loader->setFileName(fileName);

            QJsonObject &&metaJson = loader->metaData();
            QString &&iid = metaJson.value("IID").toString();

            if (iid == LLMPlugin_Meta_IID) {
                if (!loader->load()) {
                    qCWarning(logExternalLLM) << "Failed to load plugin:" << fileName 
                                            << "error:" << loader->errorString();
                    continue;
                }

                LLMPlugin *llm = dynamic_cast<LLMPlugin *>(loader->instance());
                if (!llm) {
                    qCCritical(logExternalLLM) << "Plugin file" << loader->fileName() 
                                             << "has no valid LLMPlugin instance";
                    loader->unload();
                    continue;
                }

                qCInfo(logExternalLLM) << "Successfully loaded plugin:" << fileName;
                d->llmPlugins.append(QSharedPointer<LLMPlugin>(llm));
                d->loaders.append(loader);
            }
        }
    }
    qCInfo(logExternalLLM) << "Finished reading plugins, found" << d->llmPlugins.size() << "valid plugins";
}

QList<QSharedPointer<LLMPlugin> > ExternalLLMLoader::plugins() const
{
    return d->llmPlugins;
}
