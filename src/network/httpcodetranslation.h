#ifndef HTTPCODETRANSLATION_H
#define HTTPCODETRANSLATION_H

#include <QObject>

#include <QNetworkReply>

namespace uos_ai {

class HttpCodeTranslation : public QObject
{
    Q_OBJECT
public:
    static QString translation(QNetworkReply::NetworkError error, const QString &msg);
private:
    explicit HttpCodeTranslation(QObject *parent = nullptr);
};
}
#endif // HTTPCODETRANSLATION_H
