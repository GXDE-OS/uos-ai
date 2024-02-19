#ifndef DBUSLOCALTEXTTOIMAGEREQUEST_H
#define DBUSLOCALTEXTTOIMAGEREQUEST_H

#include <QDBusInterface>
#include <QEventLoop>

class DbusLocalTextToImageRequest: public QDBusInterface
{
    Q_OBJECT

public:
    explicit DbusLocalTextToImageRequest(QObject *parent = nullptr);

    QByteArray textToImage(const QString &prompt, const QString &negativePrompt, bool isChinese);

private slots:
    void finished(const QString &);

public slots:
    void requestAborted();

private:
    QByteArray m_image;
    QEventLoop m_event;
};

#endif // DBUSLOCALTEXTTOIMAGEREQUEST_H
