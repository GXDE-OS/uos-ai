// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "modelhubllm.h"
#include "modelhubwrapper.h"

#include <QLoggingCategory>

#include <filesystem>

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)

using namespace uos_ai;

namespace fs = std::filesystem;

ModelHubLLM::ModelHubLLM(QObject *parent) : QObject(parent)
{

}

QList<LLMServerProxy> ModelHubLLM::modelList()
{
    QList<LLMServerProxy> list;

    const QString model_1_5 = "YouRong-1.5B";
    const QString model_7 = "YouRong-7B";

    auto models = ModelhubWrapper::installedModels();
    QMutexLocker lk(&mtx);

    for (auto key : wrapper.keys()) {
        if (!models.contains(key))
            wrapper.remove(key);
    }

    //改成遍历模型列表
    for (const QString &model : models) {
        if (model == model_1_5) {
            LLMServerProxy sp;
            sp.id = model_1_5;
            sp.name = tr("YouRong 1.5B");
            sp.model = LOCAL_YOURONG_1_5B;
            sp.type = LOCAL;
            sp.account.apiKey = "null";

            list << sp;
            if (!wrapper.contains(model_1_5)) {
                qCInfo(logExternalLLM) << "Found YouRong-1.5B model";
                auto ins = new ModelhubWrapper(model_1_5);
                ins->setKeepLive(true);
                wrapper.insert(model_1_5, QSharedPointer<ModelhubWrapper>(ins));
            }
        } else if (model == model_7) {
            LLMServerProxy sp;
            sp.id = model_7;
            sp.name = tr("YouRong 7B");
            sp.model = LOCAL_YOURONG_7B;
            sp.type = LOCAL;
            sp.account.apiKey = "null";

            list << sp;
            if (!wrapper.contains(model_7)) {
                qCInfo(logExternalLLM) << "Found YouRong-7B model";
                auto ins = new ModelhubWrapper(model_7);
                ins->setKeepLive(true);
                wrapper.insert(model_7, QSharedPointer<ModelhubWrapper>(ins));
            }
        } else if (!model.isEmpty() && model != "BAAI-bge-large-zh-v1.5") {
            LLMServerProxy sp;
            sp.id = model;
            sp.name = model;
            sp.model = LOCAL_OTHER_MODEL;
            sp.type = LOCAL;
            sp.account.apiKey = "null";

            list << sp;
            if (!wrapper.contains(model)) {
                qCInfo(logExternalLLM) << "Found custom model:" << model;
                auto ins = new ModelhubWrapper(model);
                ins->setKeepLive(true);
                wrapper.insert(model, QSharedPointer<ModelhubWrapper>(ins));
            }
        }
    }

    return list;
}

QSharedPointer<ModelhubWrapper> ModelHubLLM::getWrapper(const QString &id)
{
    QMutexLocker lk(&mtx);
    return wrapper.value(id);
}
