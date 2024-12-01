#include "esinglewebview.h"
#include "esinglewebview_p.h"

#include <QProcess>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QWheelEvent>
#include <QWebChannel>
#include <DGuiApplicationHelper>
#include <DPalette>

ESingleWebView::ESingleWebView(QWidget *parent)
    : QWebEngineView(parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    connect(this, &QWebEngineView::loadFinished, this, &ESingleWebView::onLoadFinished);

    m_page = new ESingleWebPage(this);
    m_page->setWebChannel(new QWebChannel(m_page));
    setPage(m_page);
    m_chat = new ESingleWebChat("", this);

    connect(m_chat, &ESingleWebChat::sigVoiceConversationStatusChanged, this, &ESingleWebView::voiceConversationStatusChanged);
}

ESingleWebView::~ESingleWebView()
{
    m_page->webChannel()->deregisterObject(m_chat);
    delete m_chat;
}

void ESingleWebView::changeAudioRecState()
{
    emit m_chat->sigAudioRecShortcutPressed();
}

void ESingleWebView::setWebViewFocusIn()
{
    emit m_chat->sigWebChatFocusIn();
}

void ESingleWebView::setWebViewFocusOut()
{
    emit m_chat->sigWebChatFocusOut();
}

void ESingleWebView::setTohiddenState()
{
    emit m_chat->sigWebchat2BeHiden();
}

void ESingleWebView::setToShowedState()
{
    emit m_chat->sigWebchat2BeShowed();
}

void ESingleWebView::setToDigitalMode()
{
    if (m_loadFinished)
        emit m_chat->sigDigitalModeActive();
    else
        addPendingTask(SwitchDigitalMode);
}

void ESingleWebView::setToChatMode()
{
    if (m_loadFinished)
        emit m_chat->sigChatModeActive();
}

void ESingleWebView::addPendingTask(Task task)
{
    m_tasks << task;
}

void ESingleWebView::setModalState(bool isModal)
{
    emit m_chat->sigWebchatModalityChanged(isModal);
}

void ESingleWebView::setWindowActiveState(bool isActive)
{
    emit m_chat->sigWebchatActiveChanged(isActive);
}

void ESingleWebView::updateFont(const QString &fontFamily, int pixelSize)
{
    if (this->m_chat) {
        m_chat->setFontInfo(fontFamily, pixelSize);
    }
}

void ESingleWebView::setWindowMode(bool isWindowMode)
{
    if (this->m_chat) {
        m_chat->setWindowMode(isWindowMode);
        emit m_chat->sigWindowModeChanged(isWindowMode);
    }
}

void ESingleWebView::onLoadFinished(bool)
{
    m_loadFinished = true;

    for (int task : m_tasks) {
        switch (task) {
        case SwitchDigitalMode:
            setToDigitalMode();
            break;
        default:
            break;
        }
    }

    m_tasks.clear();
}

bool ESingleWebView::event(QEvent *e)
{
    if (e->type() == QEvent::ChildPolished) {
        QChildEvent *ce = static_cast<QChildEvent *>(e);
        QObject *m_child = ce->child();
        if (m_child) {
            m_child->removeEventFilter(this);
            m_child->installEventFilter(this);
        }
    }

    return QWebEngineView::event(e);
}

bool ESingleWebView::eventFilter(QObject *obj, QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::FocusIn:
        setWebViewFocusIn();
        break;
    case QEvent::FocusOut:
        setWebViewFocusOut();
        break;
    default:
        break;
    }
    return QWebEngineView::eventFilter(obj, ev);
}


ESingleWebPage::ESingleWebPage(QObject *parent): QWebEnginePage(parent)
{

}

bool ESingleWebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    if (type == QWebEnginePage::NavigationTypeLinkClicked) {
        // Open the link using system's default browser
        QDesktopServices::openUrl(url);
        return false;
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}


ESingleWebChat::ESingleWebChat(const QString &scene, ESingleWebView *view)
    : EAiProxy(view), m_sceneType(scene), m_view(view)
{
    m_view->page()->webChannel()->registerObject("chatObject", this);
    m_view->installEventFilter(this);
}

void ESingleWebChat::updateVoiceConversationStatus(int status)
{
    qDebug() << "update voice conversation status, status: " << status;
    emit sigVoiceConversationStatusChanged(status);
}
