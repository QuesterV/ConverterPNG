#ifndef QUEUETS_H
#define QUEUETS_H

#include <QQueue>
#include <QMutex>
#include <QSemaphore>



//Блокирующая thread-safe очередь.
//При пустой очереди поток замораживается.
template <class T>
class QueueTS
{
public:
    void Push(const T& t)
    {
        mutex_.lock();
        queue_.enqueue(t);
        mutex_.unlock();
        sem_.release();
    }

    T Pop()
    {
        sem_.acquire();
        mutex_.lock();
        T t = queue_.dequeue();
        mutex_.unlock();
        return t;
    }

private:
    QQueue<T> queue_;
    QMutex mutex_;
    QSemaphore sem_; //семафор используется для блокировки потока
};



#endif // QUEUETS_H
