#ifndef TranServer_H
#define TranServer_H

#include <QObject>

namespace uos_ai {

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

}

#endif // TranServer_H
