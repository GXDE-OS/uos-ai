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
        , _tail(_head.load())
    {

    }

    ~TLockFreeQueue()
    {
        T value;
        while (dequeue(value)) {}
        delete _head.load();  // 删除头节点
    }

    bool isEmpty()
    {
        Obj *oldHead = _head.load();
        Obj *oldTail = _tail.load();
        if (oldHead == oldTail) {
            return true; // 队列为空
        }
        return false;
    }

    void enqueue(const T &value)
    {
        Obj *node = new Obj(value);
        while (true) {
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
        }
    }

    bool dequeue(T &value)
    {
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
