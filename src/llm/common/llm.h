#ifndef LLM_H
#define LLM_H

#include "serverdefs.h"

#include <QMap>
#include <QObject>

#define PREDICT_PARAM_STREAM "stream"            // enable stream        default:true
#define PREDICT_PARAM_TEMPERATURE "temperature"  // predict temperature  default:1.0
#define PREDICT_PARAM_SYSTEMROLE "system"        // prompt system role   default:""
#define PREDICT_PARAM_THINKCHAIN "thinkChain"    // enable llm think     default:true
#define PREDICT_PARAM_THINKINGMODE "thinkingMode"  // thinking mode param for API: "enabled" or "disabled"
#define PREDICT_PARAM_ONLINESEARCH "OnlineSearch"    // enable Online search     default:false

#define PREDICT_PARAM_INCREASEUSE "increaseUse"    // free account add use  default:false

#define PREDICT_PARAM_NOJSONOUTPUT "onJsonOutput"  // only send content not json format in dbus session

#define PREDICT_PARAM_MCPSERVERS "mcpServers"  // what mcp servers need to use.
#define PREDICT_PARAM_MCPAGENT "mcpAgentId"

// Skip AppSocketServer when the caller consumes chunks via chatTextChunkReceived signal directly
#define PREDICT_PARAM_NOSOCKET "noSocket"

class LLM : public QObject
{
    Q_OBJECT

public:
    LLM(const LLMServerProxy &serverproxy);

    virtual ~LLM();

    virtual void updateAccount(const LLMServerProxy &serverproxy);

    void loadParams(const QVariantHash &params);

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

    virtual bool isReplied() const;
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
    virtual QJsonObject predict(const QString &content, const QJsonArray &functions) = 0;

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

    void textChainContent(const QString &content);

    LLMServerProxy account() const;
protected:
    void readyThinkChainContent(const QJsonObject &content);

protected:
    bool m_replied = false;
    QVariantHash m_params;

    LLMServerProxy m_accountProxy;

private:
    int m_lastError = 0;
    QString m_lastErrorString;

    bool m_streamSwitch = false;

    QString m_createdId;
};

#endif // LLM_H
