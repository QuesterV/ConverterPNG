#include "MainWindow.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QFileDialog>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    progressUpdater.setInterval(500); //интервал обоновлени прогрессбара

    connect(&progressUpdater, &QTimer::timeout, this, &MainWindow::slt_progressUpdate_Tick); //событие: обновление прогресса
    connect(&bgWorkerWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::slt_bgWorker_Finished); //событие: конвертация закончена
}



MainWindow::~MainWindow()
{
    isCancel = true;
    delete ui;
    bgWorkerWatcher.waitForFinished(); //подождать корректной остановки основного потока конвертации
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QString dir = event->mimeData()->urls()[0].toLocalFile();
        if (QDir(dir).exists()) //проверка существования директории
            event->acceptProposedAction();
    }
}



void MainWindow::dropEvent(QDropEvent *event)
{
    QString dir = event->mimeData()->urls()[0].toLocalFile();
    ui->inpDir->setText(dir); //сохраняет путь перетаскиваемой директории
}



void MainWindow::backgroudWorker()
{//конвертирование идёт в несколько потоков, а чтение/запись в одном потоке (оптимизация расчитана на обычные HDD)
    //пользовательские настройки
    bool isSubDir = ui->chkSubdir->checkState(); //включить поддиректории
    QString directory = ui->inpDir->text(); //выбранная директория

    int inWork = 0; //счетчик подготовленных для конвертации данных

    //создание задач для конвертера из списока файлов
    QDirIterator::IteratorFlag itFlag(isSubDir ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags); //вкл/нет поддиректории
    QDirIterator dirIt(directory, QStringList("*.png"), QDir::Files | QDir::Hidden |  QDir::NoSymLinks, itFlag);
    while (dirIt.hasNext()) {
        if (isCancel) return; //отмена

        WorkerTask *task = new WorkerTask; //WorkerTask
        task->fileName = dirIt.next(); //WorkerTask: имя файла

        //считать файл
        QFile file(task->fileName);
        if (file.open(QIODevice::ReadOnly)) {
            task->pngBytes = file.readAll(); //WorkerTask: массив бит
            file.close();
        } else task->isError = true;

        multiConverter.enqueueTask(task); //добавить задачу в конвертер
        ++inWork;
        if (inWork == multiConverter.maxLoadedFiles) flushFiles(inWork); //сохранить готовые файлы
    }

    flushFiles(inWork); //сохранить оставшиейся файлы
}



void MainWindow::backgroudWorkerNoOptimize()
{//при отключенной оптимизации обработка идёт последовательно в одном потоке
    //пользовательские настройки
    bool isSubDir = ui->chkSubdir->checkState(); //включить поддиректории
    QString directory = ui->inpDir->text(); //выбранная директория

    //конвертирование
    QDirIterator::IteratorFlag itFlag(isSubDir ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags); //вкл/нет поддиректории
    QDirIterator dirIt(directory, QStringList("*.png"), QDir::Files | QDir::Hidden |  QDir::NoSymLinks, itFlag);
    while (dirIt.hasNext()) {
        if (isCancel) return; //отмена
        QString fileName = dirIt.next(); //имя файла
        QImage img;
        if (img.load(fileName) && img.save(fileName.left(fileName.lastIndexOf('.')) + ".jpg", "JPG")); //загрузка и сохранение
        else errors.append(fileName);

        ++filesDoneCount; //счетчик сконвертированных файлов
    }
}



int MainWindow::CountPngFiles(QString directory, bool isSubDir)
{
    //кол-во PNG файлов в директории
    QDir dir(directory, "*.png", QDir::NoSort, QDir::Files | QDir::Hidden |  QDir::NoSymLinks);
    int nFiles = dir.count();

    //поддиректории
    if (isSubDir) {
        QStringList listFiles = dir.entryList(QStringList("*"), QDir::Hidden | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString file, listFiles)
            nFiles += CountPngFiles(dir.absoluteFilePath(file), isSubDir);
    }

    return nFiles;
}



void MainWindow::flushFiles(int &inWork)
{
    while (inWork) {
        WorkerTask *task = multiConverter.dequeueTaskDone();

        //сохранить файл
        QFile file(task->fileName.left(task->fileName.lastIndexOf('.')) + ".jpg"); //имя файла
        if (!task->isError && file.open(QIODevice::WriteOnly)) {
            file.write(task->jpgBytes);
            file.close();
        } else task->isError = true;

        if (task->isError) errors.append(task->fileName);
        --inWork;
        delete task;
    }
}



void MainWindow::on_btnDir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory();
    if (!dir.isEmpty()) ui->inpDir->setText(dir);
}



void MainWindow::on_btnStart_clicked()
{
    if ( (!QDir( ui->inpDir->text() ).exists()) || (ui->inpDir->text() == "") ) {
        QMessageBox::warning(0, "Ошибка", "Не верно указана директория");
        return;
    }

    errors.clear(); //очистить лог ошибок
    multiConverter.resetTaskDoneCount(); //обнулить счётчик выполненных задач
    filesDoneCount = 0; //тоже, для режима "без оптимизации"
    ui->lblStatus->setText(""); //строка статуса
    isCancel = false;
    ui->btnStart->setVisible(false);
    timeWork = QDateTime::currentDateTime(); //счетчик времени

    //прогрессбар
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(CountPngFiles(ui->inpDir->text(), ui->chkSubdir->checkState())); //кол-во PNG файлов в выбранной директории
    progressUpdater.start();

    //создание основного потока конвертации (в отдельном потоке, чтобы контролы на форме не подвисали)
    if (ui->chkOptimize->checkState()) //если выбрана оптимизация
        bgWorkerWatcher.setFuture( QtConcurrent::run(this,MainWindow::backgroudWorker) );
    else //без оптимизации
        bgWorkerWatcher.setFuture( QtConcurrent::run(this,MainWindow::backgroudWorkerNoOptimize) );
}



void MainWindow::on_btnStop_clicked()
{
    isCancel = true;
}



void MainWindow::slt_progressUpdate_Tick()
{
    ui->progressBar->setValue(filesDoneCount ? filesDoneCount: multiConverter.getTaskDoneCount());
}



void MainWindow::slt_bgWorker_Finished()
{
    ui->btnStart->setVisible(true);

    if (isCancel) {
        ui->lblStatus->setText("Операция отменена");
        return;
    }

    //счетчик времени
    QDateTime t = QDateTime::currentDateTime();
    int msecs = timeWork.msecsTo(t); //милисекунды
    QString str = QDateTime::fromMSecsSinceEpoch(msecs).toUTC().toString("hh:mm:ss.zzz"); //намеренно не учитываю дни
    ui->lblStatus->setText("Время операции: "+str);

    if (!errors.isEmpty()) {
        QString str = "В процессе конвертация произошли ошибки в файлах:\n";
        for (int i = 0; i < errors.count(); i++) str += errors[i] + "\n";
        QMessageBox::warning(0, "Ошибки", str);
    }

    progressUpdater.stop(); //выкл прогрессбар
    ui->progressBar->setValue(ui->progressBar->maximum()); //отобразить прогесс 100%
}
