#ifndef IATMODEL_H
#define IATMODEL_H

#include <QObject>
#include <QVariant>

namespace uos_ai {
class IatModel : public QObject {
    Q_OBJECT
public:
    explicit IatModel(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IatModel() {}

    /**
     * @brief sendDataStart
     */
    virtual void sendDataStart() = 0;

    /**
     * @brief sendData
     */
    virtual void sendData(const QByteArray &data) = 0;

    /**
     * @brief sendDataEnd
     */
    virtual void sendDataEnd() = 0;

    /**
     * @brief processData
     */
    virtual void processData() = 0;

    /**
     * @brief processData
     */
    virtual void cancel() = 0;

signals:
    /**
     * @brief textReceived
     */
    void textReceived(QString text, bool isEnd);

    /**
     * @brief error
     */
    void error(int code, QString msg);
};
}

#endif // IATMODEL_H
