// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TTSIFLYMODEL_H
#define TTSIFLYMODEL_H

#include "modelwrapper/common/ttsmodel.h"
#include "model/modelinfo.h"

#include <QWebSocket>

namespace uos_ai {
class TtsIflyModel : public TtsModel
{
    Q_OBJECT
public:
    explicit TtsIflyModel(const QString &id,  QObject *parent = nullptr);

public:
    /**
     * @brief cancel
     */
    void cancel() override;
    /**
     * @brief sendText
     * @param text
     */
    void sendText(const QString &text, bool isStart, bool isEnd) override;

private slots:
    /**
     * @brief onTextMessageReceived
     * @param message
     */
    void onTextMessageReceived(const QString &message);

    /**
     * @brief onDisconnected
     */
    void onDisconnected();

    /**
     * @brief onConnected
     */
    void onConnected();

    /**
     * @brief continueSendText
     */
    void continueSendText();

private:
    /**
     * @brief clear
     */
    void clear();

    /**
     * @brief sendServer
     */
    void sendServer();
    /**
     * @brief splitString
     * @param inputString
     * @return
     */
    QStringList splitString(const QString &inputString);

private:
    ProviderAccount m_account;
    QSharedPointer<QWebSocket> m_web;

    int m_error = -1;

    bool m_normalExit = false;
    bool m_isEnd = false;
    QString m_text;

    QStringList m_chunkTexts;
};
}
#endif // TTSIFLYMODEL_H
