// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ttsmodel.h"

#include <QRegularExpression>

UOSAI_USE_NAMESPACE

TtsModel::TtsModel(const QString &id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
}

TtsModel::~TtsModel()
{

}

QString TtsModel::id() const
{
    return m_id;
}

void TtsModel::setModel(TtsModel::ModelType model)
{
    m_model = model;
}

TtsModel::ModelType TtsModel::getModel() const
{ 
    return m_model;
}

