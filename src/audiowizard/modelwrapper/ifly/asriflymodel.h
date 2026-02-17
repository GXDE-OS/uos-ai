// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ASRFLYMODEL_H
#define ASRFLYMODEL_H

#include "serverdefs.h"
#include "modelwrapper/common/asrmodel.h"
#include <QAudioDecoder>
UOSAI_BEGIN_NAMESPACE
class IatIflyModel;
class AsrIflyModel : public AsrModel
{
    Q_OBJECT
public:
    explicit AsrIflyModel(const QString &id,  QObject *parent = nullptr);

public slots:
    QString startAsr(const QVariantMap &param) override;
    void stopAsr() override;
    void iatSlot(QString result, bool finish);
    void bufferReady();
    void bufferfinish();
private:
    QAudioDecoder decoder;
    IatIflyModel*   m_iatModel = nullptr;
};
UOSAI_END_NAMESPACE
#endif // ASRFLYMODEL_H
