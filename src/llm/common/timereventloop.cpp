#include "timereventloop.h"

const int command_out_time = 20000;
TimerEventLoop::TimerEventLoop(QObject *parent) : QEventLoop(parent)
{
    m_msec = command_out_time;
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()),  this, SLOT(onTimeout()));
}

void TimerEventLoop::onTimeout()
{
    m_timer.stop();
    exit(EventLoopResult::EVENTLOOP_TIME_OUT);
}

int TimerEventLoop::exec(QEventLoop::ProcessEventsFlags flags)
{
    m_timer.start(m_msec);
    return QEventLoop::exec(flags);
}

void TimerEventLoop::setTimeout(int time)
{
    m_msec = time;
}

void TimerEventLoop::resetTime()
{
    m_timer.stop();
    m_timer.start(m_msec);
}
