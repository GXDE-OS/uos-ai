#ifndef ESINGLEWEBVIEW_P_H
#define ESINGLEWEBVIEW_P_H
#include "eaiproxy.h"

#include <QWebEnginePage>

/**
 * @brief 所有链接都进行外部跳转
 */
class ESingleWebPage final : public QWebEnginePage
{
    Q_OBJECT
    friend class ESingleWebView;
private:
    explicit ESingleWebPage(QObject *parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
};

/**
 * @brief 直接注册的属性，同时作为兼容过去版本的设计而存在
 */
class QMimeData;
class ESingleWebView;
class ImagePreviewDialogManager;
class ESingleWebChat final : public EAiProxy
{
    Q_OBJECT
public:
    explicit ESingleWebChat(const QString &scene, ESingleWebView *view);

signals:
    //Webchat get focus in/out signals
    void sigWebChatFocusIn();
    void sigWebChatFocusOut();
    //Audio shortcut press event
    void sigAudioRecShortcutPressed();
    //WebChat hide signal
    void sigWebchat2BeHiden();
    void sigWebchat2BeShowed();
    void sigWebchatModalityChanged(bool isModal);
    void sigWebchatActiveChanged(bool isActive);
    //Signal to active digital window
    void sigDigitalModeActive();
public slots:

private:
    QString m_sceneType;
    bool m_isLeaved = true;
    ESingleWebView *m_view = nullptr;
    int m_chatMode {0};
};

#endif // ESINGLEWEBVIEW_P_H
