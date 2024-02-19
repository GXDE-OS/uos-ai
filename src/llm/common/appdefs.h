#ifndef APPDEFS_H
#define APPDEFS_H

#include "appdbusobject.h"

#include <QJsonObject>
#include <QSharedPointer>

typedef struct AppDbusPathObject {
    QString appId;
    QString path;
    QString curLLMId;
    QVariantMap cmdPrompts;
    QSharedPointer<AppDbusObject> object;

    bool operator== (const AppDbusPathObject &param) const
    {
        return (path == param.path && curLLMId == param.curLLMId && cmdPrompts == param.cmdPrompts);
    }

    QJsonObject toJson() const
    {
        QJsonObject objJson;
        objJson.insert("appId", appId);
        objJson.insert("path", path);
        objJson.insert("curLLMId", curLLMId);
        objJson.insert("cmdPrompts", QJsonObject::fromVariantMap(cmdPrompts));

        return objJson;
    }

    void fromJson(const QJsonObject &objJson)
    {
        appId = objJson.value("appId").toString();
        path = objJson.value("path").toString();
        curLLMId = objJson.value("curLLMId").toString();
        cmdPrompts = objJson.value("cmdPrompts").toObject().toVariantMap();
    }

} AppDbusPathObject;

#endif // APPDEFS_H
