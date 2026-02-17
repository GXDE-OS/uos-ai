// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TTSMODEL_H
#define TTSMODEL_H

#include "uosai_global.h"
#include <QObject>

UOSAI_BEGIN_NAMESPACE
class TtsModel : public QObject
{
    Q_OBJECT
public:
    enum ModelType {
        Local = 0,
        Ifly
    };
public:
    explicit TtsModel(const QString &id, QObject *parent = nullptr);
    virtual ~TtsModel();

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

signals:
    /**
     * @brief error
     * @param code
     * @param errorString
     */
    void error(int code, const QString &errorString);

    /**
     * @brief appendAudioData
     */
    void appendAudioData(const QString &id, const QByteArray &data, bool isLast);

public:

    /**
     * @brief cancel
     */
    virtual void cancel() = 0;

    /**
     * @brief sendText
     * @param text
     */
    virtual void sendText(const QString &text, bool isStart, bool isEnd) = 0;

protected:
    QString m_id;
    ModelType m_model = Local;
};
UOSAI_END_NAMESPACE
#endif // TTSMODEL_H
