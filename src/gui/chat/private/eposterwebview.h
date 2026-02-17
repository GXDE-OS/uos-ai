#ifndef EPOSTERWEBVIEW_H
#define EPOSTERWEBVIEW_H

#include <QJsonObject>
#include <DWindowManagerHelper>
#include <DMainWindow>
#include <QJsonDocument>
#include <QJsonArray>
#include <QWebEngineView>

DWIDGET_BEGIN_NAMESPACE
class PosterBridgeObject : public QObject {
    Q_OBJECT
public:
    PosterBridgeObject(QObject *parent = nullptr) : QObject(parent) {}
    ~PosterBridgeObject(){
    }

public slots:
    void getPosterGCResult(const QJsonValue &data) {
        // 处理QJsonValue
        QJsonDocument doc = QJsonDocument::fromJson(data.toString().toUtf8());
        // 获取根对象
        QJsonObject rootObject = doc.object();

        // 获取data对象
        QJsonObject dataObject = rootObject.value("data").toObject();
        emit posterCreateSuccess(dataObject);
    }

    void getPosterEditResult(const QJsonValue &data) {
        // 处理QJsonValue
        QJsonDocument doc = QJsonDocument::fromJson(data.toString().toUtf8());
        // 获取根对象
        QJsonObject rootObject = doc.object();

        // 获取data对象
        QJsonObject dataObject = rootObject.value("data").toObject();
        emit posterChangeSuccess(dataObject);
    }

signals:
    void posterCreateSuccess(const QJsonObject &data);
    void posterChangeSuccess(const QJsonObject &data);
};

class EPosterWebView: public DMainWindow
{
    Q_OBJECT
public:
    enum WebSitePage {
        ModifyParameters,
        GenerateWorks = 1,
        ModifyWork,
    };

    explicit EPosterWebView(WebSitePage webstatus = WebSitePage::ModifyParameters, const QString &workId = "", QWidget *parent = nullptr);
    ~EPosterWebView();

private:
    QString m_workId = "";
    QWebEngineView *posterView;

    QByteArray downloadImageByWeb(const QUrl &url);
    void downloadImage(const QList<QString> &idList,const QList<QUrl> &urlList);
signals:
    void sigPosterCreated(const QList<QString> &idList, const QList<QByteArray> &imageData);
};

#endif // EPOSTERWEBVIEW_H

DWIDGET_END_NAMESPACE
