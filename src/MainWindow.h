#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include <QMainWindow>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>
#include <QDateTime>
#include <QFutureWatcher>
#include "MultiConverter.h"



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override; //drag drop папки (при перетаскивании)
    void dropEvent(QDropEvent *event) override; //drag drop папки (при отпускании)

private:
    Ui_MainWindow *ui; //форма
    MultiConverter multiConverter; //многопоточный конвертер
    QThread *bgWorker; //поток для загрузки/сохранения файлов (чтение/запись с обычных HDD быстрее в одном потоке)
    QTimer progressUpdater; //для прогрессбара
    QDateTime timeWork; //для замера времени выполнения
    QList<QString> errors; //лог ошибок
    int filesDoneCount = 0; //кол-во сконвертированных файлов (для прогрессбара)
    volatile bool isCancel = false; //для реализации отмены операции
    QFutureWatcher<void> bgWorkerWatcher; //объект следит за основным потоком конвертации
    void backgroudWorker(); //основная функция конвертации
    void backgroudWorkerNoOptimize(); //основная функция конвертации (без оптимизации)
    int CountPngFiles(QString directory, bool isSubDir); //возвращается кол-во PNG файлов в указанной директории.
    void flushFiles(int &inWork); //Сохраняет файлы на диск (inWork - счетчик готовых для конвертации файлов)

private slots:
    void on_btnDir_clicked(); //событие: кнопка "выбор директории"
    void on_btnStart_clicked(); //событие: кнопка Start
    void on_btnStop_clicked(); //событие: кнопка Stop
    void slt_progressUpdate_Tick(); //событие: обновление прогрессбара
    void slt_bgWorker_Finished(); //событие: конвертация закончена
};



#endif // MAINWINDOW_H
