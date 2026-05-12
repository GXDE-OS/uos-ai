// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chatdbusinterface.h"
#include "app/serverwrapper.h"
#include "app/application.h"

#include <report/knowledgefunctionpoint.h>
#include <report/eventlogutil.h>
#include <QLoggingCategory>
#include <QTimer>
#include <QDBusMetaType>

Q_DECLARE_LOGGING_CATEGORY(logDBus)

using namespace uos_ai;

ChatDBusInterface::ChatDBusInterface(ServerWrapper *parent)
    : QObject(parent)
    , QDBusContext()
    , m_server(parent)
{
    qDBusRegisterMetaType<QMap<QString, QString>>();
}

// params
// file: 上传文件的路径
// defaultPrompt: 默认提示词，用于暗文显示
void ChatDBusInterface::inputPrompt(const QString &question, const QMap<QString, QString> &params)
{
    //无人使用，暂时屏蔽
    return;
    if (question.isEmpty() && params.isEmpty()) {
        qCDebug(logDBus) << "Empty question and params, skipping input prompt";
        return;
    }
    QTimer::singleShot(0, this, [question, params]{
        QMetaObject::invokeMethod(aiApp, "inputPrompt", Qt::QueuedConnection,
            QArgument<QString>("QString", question), QArgument<QMap<QString, QString>>("QMap<QString,QString>", params));
    });
}

void ChatDBusInterface::sendToKnowledgeBase(const QStringList &file)
{
    ReportIns()->writeEvent(report::KnowledgeFunctionPoint("File Manager").assemblingData());
    if (file.isEmpty()) {
        qCDebug(logDBus) << "Empty file list, skipping knowledge base update";
        return;
    }

    qCDebug(logDBus) << "Sending files to knowledge base, count:" << file.size();
    QTimer::singleShot(0, this, [file]{
        QMetaObject::invokeMethod(aiApp, "addKnowledgeBasefile", Qt::QueuedConnection, Q_ARG(QStringList, file));
    });
}
