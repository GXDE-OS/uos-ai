#ifndef LLM_H
#define LLM_H

#include "serverdefs.h"

#include <QMap>
#include <QObject>

class LLM : public QObject
{
    Q_OBJECT

public:
    LLM(const LLMServerProxy &serverproxy);

    virtual ~LLM();

    virtual void updateAccount(const LLMServerProxy &serverproxy);

    /**
     * @brief cancel the currently executing predict
     */
    void cancel();

    /**
     * @brief setCreatedId
     */
    void setCreatedId(const QString &id);
    QString createdId() const;

    /**
     * @brief switchStream
     * @param swith
     */
    void switchStream(bool on);
    bool stream() const;

    /**
     * @brief lastError
     * @return
     */
    int lastError() const;
    void setLastError(int error);

    /**
     * @brief lastErrorString
     * @return
     */
    QString lastErrorString();
    void setLastErrorString(const QString &errorMessage);

signals:
    /**
     * @brief This signal is emitted when the task has been cancelled.
     */
    void aborted();

    /**
     * @brief readyReadChatDeltaContent
     * @param deltaData
     */
    void readyReadChatDeltaContent(const QString &deltaData);

public:
    /**
     * @brief request predictions or estimates based on the trained model
     * @param content
     * @param systemRole
     * @param temperature
     *
     * @return
     */
    virtual QJsonObject predict(const QString &content, const QJsonArray &functions, const QString &systemRole = "", qreal temperature = 1.0) = 0;

    /**
     * @brief text2Image
     * @param prompt
     * @return
     */
    virtual QList<QByteArray> text2Image(const QString &prompt, int number);

    /**
     * @brief verify
     * @return
     */
    virtual QPair<int, QString> verify() = 0;

protected:
    LLMServerProxy m_accountProxy;

private:
    int m_lastError = 0;
    QString m_lastErrorString;

    bool m_streamSwitch = false;

    QString m_createdId;
};

#endif // LLM_H
