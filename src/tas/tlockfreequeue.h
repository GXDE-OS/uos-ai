#ifndef TLOCKFREEQUEUE_H
#define TLOCKFREEQUEUE_H

#include <QAtomicPointer>
#include <QDebug>

template<typename T>
class TLockFreeQueue
{
public:
    TLockFreeQueue()
        : _head(new Obj(T()))
#ifdef COMPILE_ON_QT6
        , _tail(_head.loadRelaxed())
#else
        , _tail(_head.load())
#endif
    {

    }

    ~TLockFreeQueue()
    {
        T value;
        while (dequeue(value)) {}
#ifdef COMPILE_ON_QT6
        delete _head.loadRelaxed();  // 删除头节点
#else
        delete _head.load();  // 删除头节点
#endif
    }

    bool isEmpty()
    {
#ifdef COMPILE_ON_QT6
        Obj *oldHead = _head.loadRelaxed();
        Obj *oldTail = _tail.loadRelaxed();
#else
        Obj *oldHead = _head.load();
        Obj *oldTail = _tail.load();
#endif
        if (oldHead == oldTail) {
            return true; // 队列为空
        }
        return false;
    }

    void enqueue(const T &value)
    {
        Obj *node = new Obj(value);
        while (true) {
#ifdef COMPILE_ON_QT6
            Obj *tail = _tail.loadRelaxed();
            Obj *next = tail->next.loadRelaxed();
            if (tail == _tail.loadRelaxed()) {
                if (next == nullptr) {
                    if (tail->next.testAndSetRelaxed(nullptr, node)) {
                        _tail.storeRelaxed(node);
                        return;
                    }
                } else {
                    _tail.storeRelaxed(next);
                }
            }
#else
            Obj *tail = _tail.load();
            Obj *next = tail->next.load();
            if (tail == _tail.load()) {
                if (next == nullptr) {
                    if (tail->next.testAndSetRelaxed(nullptr, node)) {
                        _tail.store(node);
                        return;
                    }
                } else {
                    _tail.store(next);
                }
            }
#endif
        }
    }

    bool dequeue(T &value)
    {
#ifdef COMPILE_ON_QT6
        Obj *oldHead = _head.loadRelaxed();
        if (oldHead == _tail.loadRelaxed()) {
            return false; // 队列为空
        } else {
            Obj *oldNext = oldHead->next.loadRelaxed();
            if (_head.testAndSetRelaxed(oldHead, oldNext)) {
                value = oldNext->data;
                delete oldHead;  // 安全删除节点
                return true;
            }
        }
#else
        Obj *oldHead = _head.load();
        if (oldHead == _tail.load()) {
            return false; // 队列为空
        } else {
            Obj *oldNext = oldHead->next.load();
            if (_head.testAndSetRelaxed(oldHead, oldNext)) {
                value = oldNext->data;
                delete oldHead;  // 安全删除节点
                return true;
            }
        }
#endif
        return false;
    }

private:
    struct Obj {
        Obj(const T &value)
            : data(value), next(nullptr) {}
        T data;
        QAtomicPointer<Obj> next;
    };

    QAtomicPointer<Obj> _head;
    QAtomicPointer<Obj> _tail;
};

#endif // TLOCKFREEQUEUE_H
