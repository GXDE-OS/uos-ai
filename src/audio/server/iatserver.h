#ifndef IATSERVER_H
#define IATSERVER_H

#include <QObject>

class IatServer : public QObject
{
    Q_OBJECT
public:
    explicit IatServer(QObject *parent = nullptr);
    virtual ~IatServer();

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
    void textReceived(const QString &text, bool isEnd);

public:
    /**
     * @brief requestAbort
     */
    virtual void cancel() = 0;

    /**
     * @brief openServer
     */
    virtual void openServer() = 0;

    /**
     * @brief sendData
     * @param data
     */
    virtual void sendData(const QByteArray &data) = 0;

private:
    int m_model = 0;
};

#endif // IATSERVER_H
