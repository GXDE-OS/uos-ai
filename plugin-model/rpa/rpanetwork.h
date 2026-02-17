// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RPANETWORK_H
#define RPANETWORK_H

#include <QObject>

namespace uos_ai {
namespace rpa {

class RpaNetwork : public QObject
{
    Q_OBJECT
public:
    explicit RpaNetwork();

    enum ErrorType {
        NoError = 0,

        NetWorkError = 1,

        ModelError = 2
    };

public:
    QPair<int, QString> request(const QJsonObject &data, const QString &urlPath);

public slots:
    void setAbortRequest();

private slots:
    void onReadyRead();

signals:
    void sigReadStream(const QString &data);
    void sigAbort();

private:
    QPair<int, QString> generateAccessToken(const QString &clientId, const QString &clientSecret) const;

    QJsonObject parserContent(const QString &content);
};
}}
#endif // RPANETWORK_H
