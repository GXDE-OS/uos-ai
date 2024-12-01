// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FYDNNETWORK_H
#define FYDNNETWORK_H

#include <llmmodel.h>
#include <QObject>

namespace uos_ai {
namespace fydn {

class FydnNetwork : public QObject
{
    Q_OBJECT

public:
    enum ErrorType {
        NoError = 0,

        NetWorkError = 1,

        ModelError = 2
    };

    explicit FydnNetwork();

public:
    QPair<int, QString> request(const QJsonObject &data, const QString &urlPath);

public slots:
    void setAbortRequest();
    QString parseResultString(const QByteArray &result);

private slots:
    void onReadyRead();

signals:
    void sigReadStream(const QString &data);
    void sigAbort();

private:
    QPair<int, QString> generateAccessToken() const;
    QString generateSignature(const QString &jsonStr, const QString &appKey);
};

}}

#endif // FYDNNETWORK_H
