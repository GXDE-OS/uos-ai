#include "eposterwebview.h"
#include <DTitlebar>
#include <DFileIconProvider>

#include <QJsonDocument>
#include <QTimer>
#include <QWebChannel>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QNetworkReply>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QLoggingCategory>

#ifdef COMPILE_ON_QT6
#include <QWebEngineDownloadRequest>
#else
#include <QWebEngineDownloadItem>
#endif

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE

EPosterWebView::EPosterWebView(WebSitePage webstatus, const QString &workId, QWidget *parent)
    : DMainWindow(parent)
{
    m_workId = workId;
    auto configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString key;
    configPath = configPath
            + "/" + qApp->organizationName()
            + "/" + qApp->applicationName()
            + "/" + qApp->applicationName() + "-plugin.conf";

    QFileInfo configFile(configPath);
    if (!configFile.exists()) {
        //生成文件
        QFile file(configPath);
        file.open(QFile::NewOnly);
        file.close();
        QSettings* set = new QSettings(configPath, QSettings::IniFormat);
        set->beginGroup("Poster");
        set->setValue("account", "tongxintestjfwybd342hjwq");
        set->endGroup();
        key = "tongxintestjfwybd342hjwq";
        qCInfo(logAIGUI) << "Created new config file:" << configPath;
    } else {
        QSettings* set = new QSettings(configPath, QSettings::IniFormat);
        // 检查[Poster]段落是否存在
        if (set->childGroups().contains("Poster")) {
            // [Poster]段落存在，读取account值
            key = set->value("Poster/account","tongxintestjfwybd342hjwq").toString();
            qCDebug(logAIGUI) << "Using existing account from config:" << key;
        } else {
            // [Poster]段落不存在，创建该段落并设置account值
            set->beginGroup("Poster");
            set->setValue("account", "tongxintestjfwybd342hjwq");
            set->endGroup();
            key = "tongxintestjfwybd342hjwq";
            qCInfo(logAIGUI) << "Created new Poster section in config with default account";
        }
    }

    // webstatus代表进入网站哪一页
    posterView = new QWebEngineView(this);
    QWebChannel* channel = new QWebChannel(posterView->page());
     PosterBridgeObject *bridge = new PosterBridgeObject(channel);
     channel->registerObject("bridge", bridge);
     posterView->page()->setWebChannel(channel);
     switch (webstatus) {
     case WebSitePage::ModifyParameters:
         posterView->load(QUrl(QStringLiteral("https://toolsbox.photosir.cn/#/generateOutward?key=%1&source=188&gcid=%2").arg(key).arg(m_workId)));
         break;
     case WebSitePage::GenerateWorks:
         posterView->load(QUrl(QStringLiteral("https://toolsbox.photosir.cn/#/generateResult?key=%1&source=188&gcid=%2").arg(key).arg(m_workId)));
         break;
     case WebSitePage::ModifyWork:
         posterView->load(QUrl(QStringLiteral("https://toolsbox.photosir.cn/#/posterEdit?key=%1&source=188&id=%2").arg(key).arg(m_workId)));
         break;
     }
     connect(posterView->page()->profile(),&QWebEngineProfile::downloadRequested, this,
#ifdef COMPILE_ON_QT6
             [=](QWebEngineDownloadRequest* downloaditem) {
#else
             [=](QWebEngineDownloadItem* downloaditem) {
#endif

         downloaditem->accept();
     });
     connect(bridge, &PosterBridgeObject::posterCreateSuccess, this, [=](QJsonObject data){
         QJsonArray filelistArray = data.value("filelist").toArray();
         // 遍历filelist数组
         QList<QString> idList;
        QList<QUrl> urlList;
        for (const QJsonValue &value : filelistArray) {
            QJsonObject fileObject = value.toObject();
            int id = fileObject.value("id").toInt();
            QString url = fileObject.value("url").toString();
            idList.append(QString::number(id));
            urlList.append(url);
            qCDebug(logAIGUI) << "Processing file - ID:" << id << "URL:" << url;
        }

        downloadImage(idList, urlList);
    });

    connect(bridge, &PosterBridgeObject::posterChangeSuccess, this, [=](QJsonObject data){
        qCInfo(logAIGUI) << "Poster modification successful";
        QJsonArray filelistArray = data.value("filelist").toArray();
        // 遍历filelist数组
        QList<QString> idList;
        QList<QUrl> urlList;
        for (const QJsonValue &value : filelistArray) {
            QJsonObject fileObject = value.toObject();
            int id = fileObject.value("id").toInt();
            QString url = fileObject.value("url").toString();
            idList.append(QString::number(id));
            urlList.append(url);
            qCDebug(logAIGUI) << "Processing file - ID:" << id << "URL:" << url;
        }

        downloadImage(idList, urlList);
    });

    this->titlebar()->setTitle("海报助手");
    //设置后posterView等子控件会一起析构
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->titlebar()->setMenuVisible(false);
    this->titlebar()->setIcon(QIcon(":/assets/images/poster.svg"));
    this->setCentralWidget(posterView);
    this->setMinimumSize(680,300);
    this->resize(1000,700);
    this->show();
}

EPosterWebView::~EPosterWebView()
{
    qCDebug(logAIGUI) << "Cleaning up poster web view resources";
    QWebEngineProfile* engineProfile = posterView->page()->profile();
    engineProfile->clearHttpCache();        // 清理缓存
    engineProfile->clearAllVisitedLinks();  // 清理浏览记录
    QWebEngineCookieStore* pCookie = posterView->page()->profile()->cookieStore();
    pCookie->deleteAllCookies();            // 清理cookie
    pCookie->deleteSessionCookies();        // 清理会话cookie
}

QByteArray EPosterWebView::downloadImageByWeb(const QUrl &url)
{
    qCDebug(logAIGUI) << "Downloading image from URL:" << url.toString();
    
    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    // 发起请求
    QNetworkReply *reply = manager.get(request);

    // 等待请求完成
    QEventLoop loop;
    QTimer timeoutTimer;
    // 设置超时时间5秒
    const int timeoutDuration = 5000;
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeoutTimer.start(timeoutDuration);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray imageData = NULL;
    // 检查请求是否成功
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logAIGUI) << "Failed to download image:" << reply->errorString();
        reply->deleteLater();
        return imageData;
    }

    imageData = reply->readAll();
    QImage image;
    if (image.loadFromData(imageData)) {
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        image.save(&buffer, "PNG"); // 保存图片到缓冲区
        byteArray = byteArray.toBase64(); // 转换为Base64
        reply->deleteLater();
        qCDebug(logAIGUI) << "Image downloaded and converted to Base64 successfully";
        return byteArray;
    }
    
    qCWarning(logAIGUI) << "Failed to load image data";
    return "";
}

void EPosterWebView::downloadImage(const QList<QString> &idList, const QList<QUrl> &urlList) {
    qCDebug(logAIGUI) << "Starting batch image download - count:" << urlList.size();
    
    QList<QByteArray> imageDataList;
    for(const QUrl &url : urlList) {
        QByteArray imageData = downloadImageByWeb(url);
        imageDataList.append(imageData);
    }
    qCInfo(logAIGUI) << "Batch image download completed - success count:" << imageDataList.size();
    emit sigPosterCreated(idList, imageDataList);
}





