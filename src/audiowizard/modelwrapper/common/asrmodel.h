// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ASRMODEL_H
#define ASRMODEL_H

#include "uosai_global.h"
#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AsrModel : public QObject
{
    Q_OBJECT
public:
    enum ModelType {
        Local = 0,
        Ifly
    };
public:
    explicit AsrModel(const QString &id, QObject *parent = nullptr);
    virtual ~AsrModel();

    /**
     * @brief id
     * @return
     */
    QString id() const;

    /**
     * @brief setModel
     * @param model
     */
    void setModel(ModelType model);
    ModelType getModel() const;
public slots:
    virtual QString startAsr(const QVariantMap &param) = 0;
    virtual void stopAsr() = 0;
signals:
    void onNotify(const QString &msg);

protected:
    QString m_id;
    ModelType m_model = Local;
};
UOSAI_END_NAMESPACE
#endif // ASRMODEL_H

