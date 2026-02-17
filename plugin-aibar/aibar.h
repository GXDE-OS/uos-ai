#ifndef AIBAR_H
#define AIBAR_H

#include "dragmonitor.h"
#include "uosaiinterface.h"
#include "meetingassistant.h"

#include <applet.h>

namespace uos_ai {
class AIItem
{
    Q_GADGET
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QString desc READ desc)
    Q_PROPERTY(QString imageFile READ imageFile)
public:
    AIItem() = default;
    AIItem(const QString& text, const QString& desc, const QString& imageFile)
        : m_text(text),
        m_desc(desc),
        m_imageFile(imageFile) {}
    const QString& text() const { return m_text; }
    const QString& desc() const { return m_desc; }
    const QString& imageFile() const { return m_imageFile; }

private:
    QString m_text;
    QString m_desc;
    QString m_imageFile;
};

class AiBar : public DS_NAMESPACE::DApplet
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
public:
    enum DocType {
        Summary = 0,
        Translation,
        Correction,
        AddToKnowledgeBase
    };
public:
    explicit AiBar(QObject *parent = nullptr);
    Q_INVOKABLE bool visible() const;
    Q_INVOKABLE void setVisible(bool visible);
    Q_INVOKABLE bool isSupportDrop(const QString &file) const;
    Q_INVOKABLE void handleDrop(const QString &file) const;
    Q_INVOKABLE void docAction(int type, const QString &file) const;
    Q_INVOKABLE void onShowDocArea() const;
    Q_INVOKABLE QList<QVariant> getItemList() const;
    Q_INVOKABLE bool getEnableFileDrag() const;
    Q_INVOKABLE void onClickRecommend();
    Q_INVOKABLE MeetingAssistant::MeetAssistantStatus getNowMeetAssistantStatus();
    Q_INVOKABLE void onClickIcon();
Q_SIGNALS:
    void visibleChanged();
    void sigMeetAssistantStatusChanged(MeetingAssistant::MeetAssistantStatus status);

    void dragActivated(const QStringList &urls);
private:
    bool m_visible = true;
    DragMonitor drag;
    UosAiInterface uosai;
    QList<QVariant> m_itemList;

    MeetingAssistant meetingAssistant;

    void updateItemList();
};

}

#endif // AIBAR_H
