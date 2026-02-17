// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externalagentloader.h"

#include <QStandardPaths>
#include <QDirIterator>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariant>
#include <QtGlobal>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)
using namespace uos_ai;

ExternalAgentLoader::ExternalAgentLoader(QObject *parent) : QObject(parent)
{
    qCDebug(logExternalLLM) << "Initializing ExternalAgentLoader";
    
    auto local = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    if (!local.isEmpty()) {
        QString path = QString("%0/.local/share/%1/uos-ai-assistant/agent")
                .arg(local.first()).arg(qApp->organizationName());

        QFileInfo configFile(path);
        if (!configFile.exists()) {
            qCDebug(logExternalLLM) << "Creating agent directory:" << path;
            QDir(configFile.absoluteFilePath()).mkpath(".");
        }
        loadPaths.append(path);
    }

    loadPaths.append(QString("/usr/share/uos-ai-assistant/agent"));
    qCInfo(logExternalLLM) << "Agent search paths:" << loadPaths;

    watcher.addPaths(loadPaths);
    agentTimer.setInterval(2000);
    agentTimer.setSingleShot(true);
    connect(&agentTimer, &QTimer::timeout, this , &ExternalAgentLoader::agentChanged);

    connect(&watcher, &FileWatcher::fileChanged, &agentTimer , qOverload<>(&QTimer::start));
    connect(&watcher, &FileWatcher::directoryChanged, &agentTimer , qOverload<>(&QTimer::start));
}

ExternalAgentLoader::~ExternalAgentLoader()
{

}

void ExternalAgentLoader::setPaths(const QStringList &paths)
{
    loadPaths = paths;
}

void ExternalAgentLoader::readAgents()
{
    qCDebug(logExternalLLM) << "Reading agent configurations";
    allAgents.clear();

    for (const QString &path : loadPaths) {
        qCDebug(logExternalLLM) << "Searching agents in path:" << path;
        
        QDirIterator dirItera(path, { "*.desc" },
                              QDir::Filter::Files,
                              QDirIterator::IteratorFlag::NoIteratorFlags);
        while (dirItera.hasNext()) {
            dirItera.next();
            const QString &fileName { dirItera.path() + "/" + dirItera.fileName() };
            qCInfo(logExternalLLM) << "Processing agent file:" << fileName;
            
            QJsonDocument doc;
            {
                QFile file(fileName);
                if (!file.open(QFile::ReadOnly)) {
                    qCWarning(logExternalLLM) << "Failed to open agent file:" << fileName;
                    continue;
                }
                doc = QJsonDocument::fromJson(file.readAll());
            }
            
            QVariantHash root = doc.object().toVariantHash();
            auto agentList = root.value("agent").toList();
            qCDebug(logExternalLLM) << "Found" << agentList.size() << "agents in file";
            
            for (const QVariant &one : agentList) {
                QSharedPointer<ExternalAgent> obj = ExternalAgent::fromJson(one.toHash());
                if (obj.isNull()) {
                    qCWarning(logExternalLLM) << "Invalid agent configuration in file:" << fileName;
                    continue;
                }

                allAgents.append(obj);
            }
        }
    }
    
    qCInfo(logExternalLLM) << "Total agents loaded:" << allAgents.size();
}

QList<QSharedPointer<ExternalAgent> > ExternalAgentLoader::agents() const
{
    return allAgents;
}
