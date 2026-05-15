#ifndef SEAT_H
#define SEAT_H

#include <QObject>
#include <wayland-client.h>

namespace uos_ai {
namespace wayland {

class EventQueue;

class Seat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool keyboard READ hasKeyboard NOTIFY hasKeyboardChanged)
    Q_PROPERTY(bool pointer READ hasPointer NOTIFY hasPointerChanged)
    Q_PROPERTY(bool touch READ hasTouch NOTIFY hasTouchChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
    explicit Seat(QObject *parent = nullptr);
    ~Seat() override;

    bool isValid() const;
    void setup(wl_seat *seat);
    void release();
    void destroy();

    void setEventQueue(EventQueue *queue);
    EventQueue *eventQueue();

    bool hasKeyboard() const;
    bool hasPointer() const;
    bool hasTouch() const;
    QString name() const;

    operator wl_seat *();
    operator wl_seat *() const;

Q_SIGNALS:
    void hasKeyboardChanged(bool);
    void hasPointerChanged(bool);
    void hasTouchChanged(bool);
    void nameChanged(const QString &name);
    void interfaceAboutToBeReleased();
    void interfaceAboutToBeDestroyed();
    void removed();

private:
    class Private;
    QScopedPointer<Private> d;
};

}
}

#endif
