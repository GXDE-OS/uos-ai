#ifndef TranServer_H
#define TranServer_H

#include <QObject>
#include "uosai_global.h"

UOSAI_BEGIN_NAMESPACE

class TranServer : public QObject
{
    Q_OBJECT
public:
    explicit TranServer(QObject *parent = nullptr);
    virtual ~TranServer();

    /**
     * @brief setModel
     * @param model
     */
    void setModel(int model);
    int model() const;

signals:
    /**
     * @brief error
     * @param code
     * @param errorString
     */
    void error(int code, const QString &errorString);

    /**
     * @brief textReceived
     * @param text
     */
    void textReceived(const QString &text);

public:

    /**
     * @brief sendText
     * @param text
     */
    virtual void sendText(const QString &text) = 0;

private:
    int m_model = 0;
};

UOSAI_END_NAMESPACE

#endif // TranServer_H
