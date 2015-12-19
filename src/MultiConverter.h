#ifndef MULTICONVERTER_H
#define MULTICONVERTER_H

#include "QueueTS.h"
#include <QThread>



//задача для обработки конвертером
class WorkerTask
{
public:
    QByteArray pngBytes; //исходный image
    QByteArray jpgBytes; //перекодированный в jpeg image
    QString fileName; //имя файла
    bool isError = false; //при обработке задачи произошла ошибка
};



//многопоточный конвертер (кодирует/декодирует image)
class MultiConverter
{
public:
    const int maxLoadedFiles = 20; //максимальное кол-во загружаемых в память файлов для конвертирования (для ограничения используемой памяти)
    MultiConverter();
    ~MultiConverter();
    void enqueueTask(WorkerTask *task); //добавляет задачу в конец очереди
    WorkerTask* dequeueTaskDone(); //удаляет и возвращает выполненную задачу из начала очереди
    int getTaskDoneCount(); //возвращает общее кол-во выполненных задач
    void resetTaskDoneCount(); //сбрасывает счётчик выполненных задач

private:
    QThread **threads_; //массив рабочих потоков
    QueueTS<WorkerTask*> tasks_; //очередь ожидающих выполнения задач
    QueueTS<WorkerTask*> tasksDone_; //очередь выполненных задач
    QAtomicInt taskDoneCount_ = 0; //общее кол-во выполненных задач
    void work(); //основная функция конвертации

    class ThreadWorker : public QThread //класс нужен для запуска потока QThread
    {
    public:
        ThreadWorker(MultiConverter &multiConverter);
        void run();
    private:
        MultiConverter &multiConverter_;
    };
};



#endif // MULTICONVERTER_H
