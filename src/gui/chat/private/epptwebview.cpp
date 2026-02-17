#include "epptwebview.h"
#include <DTitlebar>
#include <DFileIconProvider>
#include <DFileDialog>

#include <QJsonDocument>
#include <QTimer>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QNetworkReply>
#include <QBuffer>
#include <QStandardPaths>
#include <QLoggingCategory>

#ifdef COMPILE_ON_QT6
#include <QWebEngineDownloadRequest>
#else
#include <QWebEngineDownloadItem>
#endif

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DWIDGET_USE_NAMESPACE

EPPTWebView::EPPTWebView(const QString &content, const QString &step, const QString &workId, QWidget *parent)
    : DMainWindow(parent)
{
    m_content = content;
    m_step = step;
    m_workId = workId;

    if (m_content != "") {
        m_content = removeFirstLine(m_content);
        qCDebug(logAIGUI) << "Content first line removed";
    }

    // step代表进入网站哪一页
    QWebEngineView *pptView = new QWebEngineView(this);
    QWebChannel* channel = new QWebChannel(pptView->page());
    PPTBridgeObject *bridge = new PPTBridgeObject(channel);
    // 设置channel并将bridge对象注册到channel
    channel->registerObject("myObject", bridge);
    pptView->page()->setWebChannel(channel);
    
    QString url = QStringLiteral("https://uosai.uniontech.com/assistant/aippt/");
    qCDebug(logAIGUI) << "Loading PPT assistant URL:" << url;
    pptView->load(QUrl(url));
    
    connect(pptView->page()->profile(),&QWebEngineProfile::downloadRequested, this,
#ifdef COMPILE_ON_QT6
            [=](QWebEngineDownloadRequest* downloaditem) {
#else
            [=](QWebEngineDownloadItem* downloaditem) {
#endif
        QString itemPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/";
#ifdef COMPILE_ON_QT6
        QString downloadPath = downloaditem->downloadDirectory() + "/" + downloaditem->downloadFileName();
#else
        QString downloadPath = downloaditem->path();
#endif
        QFileInfo fileInfo(downloadPath);
        QString itemName = fileInfo.fileName();
        itemPath += itemName;        
        QString itemFilePath = DFileDialog::getSaveFileName(
            this,
            tr("Export Item As"),
            itemPath);
        if (!itemFilePath.isEmpty()) {
#ifdef COMPILE_ON_QT6
            fileInfo = QFileInfo(itemFilePath);
            downloaditem->setDownloadDirectory(fileInfo.absolutePath());
            downloaditem->setDownloadFileName(fileInfo.fileName());
#else
            downloaditem->setPath(itemFilePath);
#endif
            qCDebug(logAIGUI) << "Download accepted - saving to:" << itemFilePath;
            downloaditem->accept();
        } else {
            qCDebug(logAIGUI) << "Download cancelled by user";
            downloaditem->cancel();
        }
    });
    connect(pptView,&QWebEngineView::loadFinished, this, [=](){
        qCDebug(logAIGUI) << "Web page loaded, injecting content and parameters";
        pptView->page()->runJavaScript(QString("var custom_generate_content = `%1`;var m_step = %2;var m_workid = %3;").arg(m_content).arg(m_step).arg(m_workId));
    });
    connect(bridge, &PPTBridgeObject::pptCreateSuccess, this, [=](QJsonObject data){
        int id = data.value("id").toInt();
        QString thumbnail = data.value("thumbnail").toString();
        qCInfo(logAIGUI) << "PPT creation successful - ID:" << id;
        downloadImage(QString::number(id), QUrl(thumbnail));
    });
    
    connect(bridge, &PPTBridgeObject::pptThumbnailChanged, this, [=](QJsonObject data){
        int id = data.value("id").toInt();
        QString thumbnail = data.value("thumbnail").toString();
        qCInfo(logAIGUI) << "PPT thumbnail changed - ID:" << id;
        changeImage(QString::number(id), QUrl(thumbnail));
    });

    this->titlebar()->setTitle("PPT 助手");
    //设置后pptView等子控件会一起析构
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->titlebar()->setMenuVisible(false);
    this->titlebar()->setIcon(QIcon(":/assets/images/ppt.svg"));
    this->setCentralWidget(pptView);
    this->setMinimumSize(680,300);
    this->resize(1000,700);
    this->show();
}

EPPTWebView::~EPPTWebView()
{
    qCDebug(logAIGUI) << "Cleaning up PPT web view resources";
}

QString EPPTWebView::removeFirstLine(const QString &text)
{
    qCDebug(logAIGUI) << "Removing first line from content";
    // 使用正则表达式分割字符串
#ifdef COMPILE_ON_QT6
    QStringList lines = text.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
#else
    QStringList lines = text.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
#endif
    lines.removeFirst();

    // 将剩余的行重新组合成一个 QString
    QString result;
    foreach (const QString &line, lines) {
        result += line + "\n"; // 根据需要添加适当的行分隔符
    }
    return result;
}

QByteArray EPPTWebView::downloadImageByWeb(const QUrl &url)
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
        image.save(&buffer, "JPEG"); // 保存图片到缓冲区
        byteArray = byteArray.toBase64(); // 转换为Base64
        reply->deleteLater();
        qCDebug(logAIGUI) << "Image downloaded and converted to Base64 successfully";
        return byteArray;
    }
    
    qCWarning(logAIGUI) << "Failed to load image data";
    return "";
}

void EPPTWebView::downloadImage(const QString &id, const QUrl &url) {
    qCDebug(logAIGUI) << "Starting image download for PPT ID:" << id;
    QByteArray imageData = downloadImageByWeb(url);
    QList<QByteArray> imageDataList;
    imageDataList.append(imageData);
    emit sigPPTCreated(id, imageDataList);
}

void EPPTWebView::changeImage(const QString &id, const QUrl &url) {
    qCDebug(logAIGUI) << "Starting image change for PPT ID:" << id;
    QByteArray imageData = downloadImageByWeb(url);
    QList<QByteArray> imageDataList;
    imageDataList.append(imageData);
    emit sigPPTChanged(id, imageDataList);
}





