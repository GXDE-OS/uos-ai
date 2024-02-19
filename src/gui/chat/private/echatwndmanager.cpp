#include "echatwndmanager.h"

#include <QWidget>
#include <QEvent>
#include <QDebug>

EChatWndManager::EChatWndManager(QObject *parent)
    : QObject{parent}
{
}

EChatWndManager::WinInfo::WinInfo(QWidget *w)
    : window(w)
{
}

bool EChatWndManager::WinInfo::operator ==(const WinInfo &other) const
{
    return window == other.window;
}

bool operator == (QWidget *window, const EChatWndManager::WinInfo &other)
{
    return window == other.window;
}

EChatWndManager::WinInfo::~WinInfo() = default;
EChatWndManager::WinInfo::WinInfo(const WinInfo &) = default;

bool EChatWndManager::eventFilter(QObject *obj, QEvent *event)
{
    auto receiver = dynamic_cast<QWidget *>(obj);

    switch (event->type()) {
    case QEvent::Show: {
        updateModalWindow(receiver, true);
    } break;
    case QEvent::Close: {
        updateModalWindow(receiver, false);
    } break;
    case QEvent::Hide: {
        updateModalWindow(receiver, false);
    } break;
    case QEvent::DeferredDelete: {
        unregisteWindow(receiver);
    } break;
    default:
        break;
    };

    return false;
}

void EChatWndManager::updateModalWindow(QWidget *w, bool visable)
{
    auto iter = m_windowMap.find(w);

    int preModalCount = m_modalStack.s.size();
    int currModalCount = preModalCount;

    if (visable) {
        auto iter = m_windowMap.find(w);

        if (iter != m_windowMap.end()) {
            //make sure only add one ref
            //to same window
            if (w->isModal()) {
                m_modalStack.top(w);
                iter->second.isModal = true;
                currModalCount = m_modalStack.count();
            }
        }
    } else {
        if (iter != m_windowMap.end()) {
            if (iter->second.isModal) {
                iter->second.isModal = false;
                m_modalStack.remove(w);
                currModalCount = m_modalStack.count();
            }
        }
    }

    //First modal window popup
    if (preModalCount == 0 && currModalCount == 1) {
        emit modalStateChanged(true);
    } else if (preModalCount == 1 && currModalCount == 0) {
        //Last modal window close
        emit modalStateChanged(false);
    }

//    qInfo() << "preModalCount= " << preModalCount
//            << " currModalCount=" << currModalCount;
}

void EChatWndManager::registeWindow(QWidget *w)
{
    auto iter = m_windowMap.find(w);
    if (iter == m_windowMap.end()) {
        auto result =
            m_windowMap.emplace(std::piecewise_construct, std::forward_as_tuple(w),
                                std::forward_as_tuple(w));
        iter = result.first;

        iter->first->installEventFilter(this);
    }
}

void EChatWndManager::unregisteWindow(QWidget *w)
{
    auto iter = m_windowMap.find(w);

    if (iter != m_windowMap.end()) {
        iter->first->removeEventFilter(this);

        m_windowMap.erase(iter);
    }
}

int EChatWndManager::EModalStack::find(QWidget *w)
{
    int pos = -1;

    for (int i = 0; i < s.size(); i++) {
        if (s.at(i) == w) {
            pos = i;
            break;
        }
    }

    return pos;
}

void EChatWndManager::EModalStack::top(QWidget *w)
{
    int pos = find(w);

    if (pos < 0) {
        s.push_front(w);
    }
}

void EChatWndManager::EModalStack::remove(QWidget *w)
{
    int pos = find(w);

    if (pos >= 0) {
        s.removeAt(pos);
    }
}

int EChatWndManager::EModalStack::count()
{
    return s.size();
}
