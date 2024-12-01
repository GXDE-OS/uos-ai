// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fydnplugin.h"
#include "zgfyllm.h"

#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

using namespace uos_ai;
using namespace uos_ai::fydn;

static const char *kRoleQA = "Q&A Service"; //类案查询
static const char *kRoleRI = "Regulatory inquiry"; //法规查询

FydnPlugin::FydnPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList FydnPlugin::modelList() const
{
    return {ZgfyLLM::modelID()};
}

QStringList FydnPlugin::roles(const QString &model) const
{
    if (model != ZgfyLLM::modelID())
        return {};

    return {kRoleRI, kRoleQA};
}

QVariant FydnPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == ZgfyLLM::modelID())
            return ZgfyLLM::modelID();
        else if (id == kRoleQA)
            return QString("法律助手-类案查询");
        else if (id == kRoleRI)
            return QString("法律助手-法规查询");
    } else if (query == QUERY_ICON_NAME) {
        if(id == ZgfyLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/llm-fayan.svg";
        else if (id == kRoleQA)
            return "legal-assistant_cases";
        else if (id == kRoleRI)
            return "legal-assistant_laws";
        else
            return QVariant();
    } else if (query == QUERY_DESCRIPTION) {
        if (id == kRoleQA)
            return "根据输入的案情描述，检索查询事实相似的案例。";
        else if (id == kRoleRI)
            return "根据用户的检索问题为用户推荐相关法条，支持精确法条查询和基于案情法条查询。";
        else
            return QVariant();
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
    } else if (query == QUERY_QUESTIONS) {
        if (id == kRoleQA) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_caseInquiryFAQ) {
                jsonArray.append(obj);
            }
            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        } else if (id == kRoleRI) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_regulatoryInquiryFAQ) {
                jsonArray.append(obj);
            }
            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        } else {
            return QVariant();
        }
    }

    return QVariant();
}

LLMModel *FydnPlugin::createModel(const QString &name)
{
    if (name == ZgfyLLM::modelID())
        return new ZgfyLLM;

    return nullptr;
}

void FydnPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-regulatory-inquiry.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open regulatory-inquiry FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "regulatory-inquiry FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_regulatoryInquiryFAQ.append(faqObject);
        }
    }

    QFile caseFile(QString(ASSETS_INSTALL_DIR) + "assistant-case-inquiry.json");
    if (!caseFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open case-inquiry FAQ file.";
        return;
    }
    jsonData = caseFile.readAll();
    caseFile.close();

    jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "case-inquiry FAQ file format error!";
        return;
    }

    jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_caseInquiryFAQ.append(faqObject);
        }
    }
}
