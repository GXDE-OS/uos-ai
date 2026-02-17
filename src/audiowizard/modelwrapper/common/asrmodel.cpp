// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "asrmodel.h"

#include <QRegularExpression>

UOSAI_USE_NAMESPACE

AsrModel::AsrModel(const QString &id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
}

AsrModel::~AsrModel()
{

}

QString AsrModel::id() const
{
    return m_id;
}

void AsrModel::setModel(AsrModel::ModelType model)
{
    m_model = model;
}

AsrModel::ModelType AsrModel::getModel() const
{
    return m_model;
}

