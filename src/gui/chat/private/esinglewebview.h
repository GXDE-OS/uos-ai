#ifndef ESingleWebView_H
#define ESingleWebView_H

#include <QWebEngineView>
#include <QJsonObject>
#include <DGuiApplicationHelper>

class ESingleWebPage;
class ESingleWebChat;
class ESingleWebView: public QWebEngineView
{
    Q_OBJECT
    friend class ESingleWebChat;
public:
    enum Task {
        SwitchDigitalMode = 0,
    };

    explicit ESingleWebView(QWidget *parent = nullptr);

    ~ESingleWebView() override;

public:
    void changeAudioRecState();

    void setWebViewFocusIn();

    void setWebViewFocusOut();

    void setTohiddenState();

    void setToShowedState();

    void setToDigitalMode();
    void setToChatMode();

    void addPendingTask(Task task);

    void setModalState(bool isModal);

    void setWindowActiveState(bool isActive);

    void updateFont(const QString &fontFamily, int pixelSize);

    void setWindowMode(bool isWindowMode = false);

    bool chatInitFinished();

signals:
    void voiceConversationStatusChanged(int status);
    void docSummaryDragInView(const QStringList &docPaths);

    void sigChatInitFinished();

private slots:
    void onLoadFinished(bool ok);
    void onChatInitFinished();

protected:
    bool event(QEvent *e) override;

    bool eventFilter(QObject *obj, QEvent *ev) override;

    void dropEvent(QDropEvent *event) override;

private:
    bool m_loadFinished = false;
    bool m_chatInitFinished = false;
    QList<int> m_tasks;

    ESingleWebPage *m_page = nullptr;
    ESingleWebChat *m_chat = nullptr;
};

#endif // ESingleWebView_H
