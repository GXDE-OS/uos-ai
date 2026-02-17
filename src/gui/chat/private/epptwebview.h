#ifndef EPPTWEBVIEW_H
#define EPPTWEBVIEW_H

#include <QJsonObject>
#include <DWindowManagerHelper>
#include <DMainWindow>

DWIDGET_BEGIN_NAMESPACE
class PPTBridgeObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString apiSecret READ getApiSecret CONSTANT)
    Q_PROPERTY(QString apiKey READ getApiKey CONSTANT)
public:
    PPTBridgeObject(QObject *parent = nullptr) : QObject(parent) {}
    ~PPTBridgeObject(){
        qInfo()<<"~PPTBridgeObject===";
    }

public slots:
    void processObject(const QString &eventType,const QJsonObject &data) {
        // 处理QJsonValue
        if (eventType == "GENERATE_PPT_SUCCESS") {
            pptCreateSuccess(data);
        }
        if (eventType == "PPT_SAVE" && data.contains("thumbnail")) {
            pptThumbnailChanged(data);
        }
    }
    QString getApiSecret() const {
        return "2YdWhPr6WYyWkPgZYQSuEILduOe4MvyI";
    }
    QString getApiKey() const {
        return "6684d4c711462";
    }
signals:
    void pptCreateSuccess(const QJsonObject &data);
    void pptThumbnailChanged(const QJsonObject &data);
};
class EPPTWebView: public DMainWindow
{
    Q_OBJECT
public:

    explicit EPPTWebView(const QString &content = "", const QString &step = "", const QString &workId = "", QWidget *parent = nullptr);
    ~EPPTWebView() override;
private:
   QString m_content = "";
   QString m_step = "";
   QString m_workId = "";

   QString removeFirstLine(const QString &text);
   QByteArray downloadImageByWeb(const QUrl &url);
   void downloadImage(const QString &id,const QUrl &url);
   void changeImage(const QString &id,const QUrl &url);

signals:
   void sigPPTCreated(const QString &id, const QList<QByteArray> &imageData);
   void sigPPTChanged(const QString &id, const QList<QByteArray> &imageData);
};

#endif // EPPTWebView_H

DWIDGET_END_NAMESPACE
