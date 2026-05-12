// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "modelhubwrapper.h"

#include <QString>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcess>

#include <unistd.h>

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)
using namespace uos_ai;

ModelhubWrapper::ModelhubWrapper(const QString &model, QObject *parent)
    : QObject(parent)
    , modelName(model)
{
    Q_ASSERT(!model.isEmpty());
}

ModelhubWrapper::~ModelhubWrapper()
{
    if (!keepLive && pid > 0) {
        int rpid = modelStatus(modelName).value("pid").toInt();

        // start by me, to kill.
        if (rpid == pid) {
            qCInfo(logExternalLLM) << "Killing modelhub server" << modelName << "with pid" << pid;
            system(QString("kill -3 %0").arg(pid).toStdString().c_str());
        } else {
            qCInfo(logExternalLLM) << "Server" << modelName << pid << "was not launched by this instance";
        }
    }
}

bool ModelhubWrapper::isRunning()
{
    QString statFile = QString("/tmp/deepin-modelhub-%0/%1.state").arg(getuid()).arg(modelName);
    bool ret = QFileInfo::exists(statFile);
    return ret;
}

bool ModelhubWrapper::ensureRunning()
{
    if (health())
        return true;

    // check running by user
    QWriteLocker lk(&lock);
    {
        updateHost();
        if (!host.isEmpty() && port > 0)
            return true;
    }

    qCDebug(logExternalLLM) << "Starting modelhub server for model:" << modelName;

    const int idle = 180;
    QProcess process;
    process.setProgram("deepin-modelhub");
    process.setArguments({"--run", modelName, "--exit-idle", QString::number(idle)}); // 3分钟自动退出
    bool ok = process.startDetached(&pid);
    if (!ok || pid < 1) {
        qCCritical(logExternalLLM) << "Failed to start modelhub server:" 
                                 << process.program() << process.arguments()
                                 << "Error:" << process.errorString();
        return false;
    }

    qCInfo(logExternalLLM) << "Successfully started modelhub server for" << modelName << "with pid" << pid;

    // wait server
    {
        const QString proc = QString("/proc/%0").arg(pid);
        int waitCount = 60 * 5; // 等1分钟加载模型
        while (waitCount-- && QFileInfo::exists(proc)) {
            QThread::msleep(200);
            updateHost();
            if (!host.isEmpty() && port > 0) {
                qCInfo(logExternalLLM) << "Modelhub server ready for" << modelName 
                        << "at" << host << ":" << port;
                return true;
            }
        }
    }

    return false;
}

bool ModelhubWrapper::health()
{
    QReadLocker lk(&lock);
    if (host.isEmpty() || port < 1)
        return false;
    lk.unlock();

    QUrl url = urlPath("/health");

    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);
    if (!reply)
        return false;

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    return reply->error() == QNetworkReply::NoError;
}

void ModelhubWrapper::setKeepLive(bool live)
{
    keepLive = live;
}

QString ModelhubWrapper::urlPath(const QString &api) const
{
    QReadLocker lk(&lock);
    return QString("http://%0:%1%2").arg(host).arg(port).arg(api);
}

bool ModelhubWrapper::isModelhubInstalled()
{
    QString out;
    if (openCmd("deepin-modelhub -v", out))
        return true;

    return false;
}

bool ModelhubWrapper::isModelInstalled(const QString &model)
{
    if (model.isEmpty())
        return false;

    auto list = ModelhubWrapper::installedModels();
    return list.contains(model, Qt::CaseInsensitive);
}

QVariantHash ModelhubWrapper::modelStatus(const QString &model)
{
    if (model.isEmpty())
        return {};

    QList<QVariantHash> infos =  ModelhubWrapper::modelsStatus();
    for (auto mvh : infos) {
        QString name = mvh.value("model").toString();
        if (name.compare(model, Qt::CaseInsensitive) == 0){
            return mvh;
        }
    }

    return {};
}

QList<QVariantHash> ModelhubWrapper::modelsStatus()
{
    QList<QVariantHash> ret;
    QString out;
    if (openCmd("deepin-modelhub --list server --info", out)) {
        auto vh = QJsonDocument::fromJson(out.toUtf8()).object().toVariantHash();
        auto ml = vh.value("serverinfo").value<QVariantList>();
        for (const QVariant &var: ml) {
            auto mvh = var.value<QVariantHash>();
            ret.append(mvh);
        }
    }

    return ret;
}

bool ModelhubWrapper::openCmd(const QString &cmd, QString &out)
{
    FILE* pipe = popen(cmd.toStdString().c_str(), "r");
    if (!pipe) {
        qCCritical(logExternalLLM) << "Failed to execute command:" << cmd;
        return false;
    }

    char buffer[256] = {0};
    QString tmp;
    while (fgets(buffer,  sizeof(buffer) - 1, pipe) != nullptr)
        tmp.append(buffer);

    if (tmp.isEmpty()) {
        pclose(pipe);
        qCWarning(logExternalLLM) << "Command produced no output:" << cmd;
        return false;
    }
    pclose(pipe);
    out = tmp;
    return true;
}

QStringList ModelhubWrapper::installedModels()
{
    QString out;
    QStringList modelList;
    if (ModelhubWrapper::openCmd("deepin-modelhub --list model --info", out)) {
        auto vh = QJsonDocument::fromJson(out.toUtf8()).object().toVariantHash();
        modelList = vh.value("model").toStringList();
        auto modelInfo = vh.value("details").toHash();
        for (auto it = modelInfo.begin(); it != modelInfo.end(); ++it) {
            auto value = it.value().toHash();
            auto archs = value.value("architectures").toStringList();
            if (!archs.contains("LLM", Qt::CaseInsensitive))
                modelList.removeAll(it.key());
        }
    }

    return modelList;
}

void ModelhubWrapper::updateHost()
{
    auto vh = modelStatus(modelName);
    host = vh.value("host").toString();
    port = vh.value("port").toInt();
}
