#ifndef TIMEREVENTLOOP_H
#define TIMEREVENTLOOP_H

#include <QEventLoop>
#include <QTimer>

class TimerEventLoop : public QEventLoop
{
    Q_OBJECT

public:
    enum EventLoopResult {
        EVENTLOOP_SUCCESS = 0,
        EVENTLOOP_TIME_OUT = Qt::UserRole,
        EVENTLOOP_USER_CANCEL = Qt::UserRole + 1,
        EVENTLOOP_END
    };

    explicit TimerEventLoop(QObject *parent = nullptr);

public:
    void setTimeout(int time);
    void resetTime();
    int exec(ProcessEventsFlags flags = AllEvents);

private slots:
    void onTimeout();

private:
    int m_msec;
    QTimer m_timer;
};

#endif // TIMEREVENTLOOP_H
