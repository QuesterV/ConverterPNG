#include "MultiConverter.h"
#include <QImage>
#include <QBuffer>



MultiConverter::MultiConverter()
{
    int n = QThread::idealThreadCount(); //кол-во ядер в системе

    //создание рабочих потоков
    threads_ = new QThread*[n];
    for (int i = 0; i < n; ++i) {
        threads_[i] = new ThreadWorker(*this);
        threads_[i]->start();
    }
}



MultiConverter::~MultiConverter()
{
    int n = QThread::idealThreadCount(); //кол-во ядер в системе

    for (int i = 0; i < n; ++i) tasks_.Push(nullptr); //разблокировка ожидающих в очереди потоков

    //дождаться завершния потоков и удалить объекты
    for (int i = 0; i < n; ++i) {
        threads_[i]->wait();
        delete threads_[i];
    }
    delete threads_;
}



WorkerTask* MultiConverter::dequeueTaskDone()
{
    return tasksDone_.Pop();
}



int MultiConverter::getTaskDoneCount()
{
    return taskDoneCount_;
}



void MultiConverter::resetTaskDoneCount()
{
    taskDoneCount_.fetchAndStoreOrdered(0);
}



void MultiConverter::enqueueTask(WorkerTask *task)
{
    tasks_.Push(task);
}



void MultiConverter::work()
{
    while (true) {
        //получить из очереди задачу
        WorkerTask *task = tasks_.Pop(); //очередь можно не проверять на пустую коллекцию, т.к. семафор пропустит только если в очереди есть задания
        if (task == nullptr) return; //окончание работы

        //конвертация PNG->JPG
        QImage img;
        if (img.loadFromData(task->pngBytes)) {
            QBuffer buffer(&task->jpgBytes);
            buffer.open(QIODevice::WriteOnly);
            if (!img.save(&buffer, "JPG")) task->isError = true;
            buffer.close();
        } else task->isError = true;

        task->pngBytes.clear(); //больше не понадобится

        tasksDone_.Push(task); //список выполненный задач
        taskDoneCount_.fetchAndAddOrdered(1); //атомарно увеличить счётчик выполненных задач
    }
}


//==================================


MultiConverter::ThreadWorker::ThreadWorker(MultiConverter &multiConverter) :
    QThread(),
    multiConverter_(multiConverter)
{
}



void MultiConverter::ThreadWorker::run()
{
    multiConverter_.work();
}
