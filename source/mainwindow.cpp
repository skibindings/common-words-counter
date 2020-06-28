#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <QFileDialog>
#include <QProcess>
#include <QtDebug>
#include <QFuture>
#include <future>
#include <thread>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->remove_selected_button->setEnabled(false);
    ui->analyze_selected_button->setEnabled(false);
    qDebug() << "App path : " << qApp->applicationDirPath();
    ui->progressBar->hide();

    connect(&watcher, SIGNAL(finished()), this, SLOT(finished_analyze()));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("Windows-1251"));
    analyze_work = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_load_file_button_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this,"Загрузить файл", "C://");
    if(file_name.size() > 0) {
        bool stop = false;
        for(int i = 0; i < ui->file_list->count(); i++) {
            //qDebug(ui->file_list->selectedItems()[i]->text().toUtf8());
            if(ui->file_list->item(i)->text().compare(file_name) == 0) {
               stop = true;
               break;
            }
        }
        if(!stop)
            ui->file_list->addItem(file_name);
    }
}

void MainWindow::on_file_list_itemClicked(QListWidgetItem *item)
{
    if(ui->file_list->selectedItems().size() > 0) {
        ui->remove_selected_button->setEnabled(true);
        if(!analyze_work)
            ui->analyze_selected_button->setEnabled(true);
    }
    else {
        ui->remove_selected_button->setEnabled(false);
        ui->analyze_selected_button->setEnabled(false);
    }
    QApplication::processEvents();
}

void MainWindow::on_remove_selected_button_clicked()
{
    QModelIndexList selectedList = ui->file_list->selectionModel()->selectedIndexes(); // take the list of selected indexes
    std::sort(selectedList.begin(),selectedList.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    for(const QModelIndex& singleIndex : selectedList)
        ui->file_list->model()->removeRow(singleIndex.row());

    ui->remove_selected_button->setEnabled(false);
    ui->analyze_selected_button->setEnabled(false);
}

int MainWindow::analyze(int items_number, int set_size,  QProgressBar *pb) {

    qDebug() << "Process laucnhed";

    QProcess*  process_analyze = new QProcess();
    QString program = "mystem.exe";

    double progress = 0.0;

    for(int i = 0; i < items_number; i++) {
        QStringList arguments;
        arguments << "-d" << "-nl";
        arguments << "-sps";
        arguments << files_to_analyze[i];
        QString out_file = QString("pre_analyze_%1.txt").arg(i+1);
        arguments << out_file;
        qDebug() <<  arguments;
        process_analyze->start(program,arguments);
        process_analyze->waitForFinished();
        /*
        while(!process_analyze->atEnd()) {
            if(progress < (double)(i+1)/(double)items_number*75.0) {
                progress += 0.05;
            }
            pb->setValue(progress);
        }
        */
        pb->setValue((double)(i+1)/(double)items_number*75.0);

    }

    double percents_in_file = (99.0-75.0) / (double)items_number;
    QMap<QString, int> temp_map = QMap<QString,int>();

    bool stop = false;
    int file_number = 1;
    while(!stop) {
        pb->setValue(75+(file_number-1)*percents_in_file);
        QString fileName = QString("pre_analyze_%1.txt").arg(file_number);
        QFile inputFile(fileName);
        if (inputFile.open(QIODevice::ReadOnly)) {
           QTextStream in(&inputFile);
           in.setCodec("UTF-8");

           while (!in.atEnd()) {
              QString line = in.readLine();

              if(temp_map.contains(line)) {
                  temp_map[line]++;
              }
              else {
                  temp_map.insert(line,1);
              }
           }
           file_number++;
           inputFile.close();
        }
        else {
            return 1;
        }
        if(file_number == items_number+1) {
            stop = true;
        }
    }


    for(int j = 0; j < set_size; j++) {
        QMapIterator<QString, int> i(temp_map);
        int max_entires = 0;
        QString max_entries_string;
        while (i.hasNext()) {
            i.next();
            if(i.value() > max_entires) {
                max_entires = i.value();
                max_entries_string = i.key();
            }
        }
        analyze_result.insert(max_entries_string,max_entires);
        temp_map.remove(max_entries_string);
    }

    pb->setValue(99);

    return 0;
}

QList<QString> MainWindow::files_to_analyze = QList<QString>();
QMap<QString, int> MainWindow::analyze_result = QMap<QString,int>();

void MainWindow::finished_analyze() {
    qDebug() << "Finished analyze";
    if(watcher.result() == 0) {
        ui->statusbar->showMessage("Анализ закончен!",2000);
        for(int j = 0; j < ui->set_size_field->value(); j++) {
            QMapIterator<QString, int> i(analyze_result);
            int max_entires = 0;
            QString max_entries_string;
            while (i.hasNext()) {
                i.next();
                if(i.value() > max_entires) {
                    max_entires = i.value();
                    max_entries_string = i.key();
                }
            }
            QString str = QString(max_entries_string+" : %1");
            ui->result_list->addItem(str.arg(max_entires));

            analyze_result.remove(max_entries_string);
        }
    }
    else {
        QMessageBox::critical(this,"Ошибка","Анализ прерван");
        ui->statusbar->showMessage("Анализ прерван!",2000);
    }
    analyze_work = false;
    if(ui->file_list->selectedItems().size() > 0) {
        ui->analyze_selected_button->setEnabled(true);
    }
    ui->analyze_all_button->setEnabled(true);
    ui->set_size_field->setEnabled(true);

    ui->progressBar->hide();
}

void MainWindow::on_analyze_all_button_clicked()
{
    ui->result_list->clear();
    analyze_result.clear();
    files_to_analyze.clear();

    analyze_work = true;

    ui->statusbar->showMessage("Анализ...");

    for(int i = 0; i < ui->file_list->count(); i++) {
        files_to_analyze.append(ui->file_list->item(i)->text());
    }

    ui->analyze_selected_button->setEnabled(false);
    ui->analyze_all_button->setEnabled(false);
    ui->set_size_field->setEnabled(false);

    ui->progressBar->show();
    ui->progressBar->setValue(0);

    QFuture<int> future = QtConcurrent::run(analyze,ui->file_list->count(),ui->set_size_field->value(),ui->progressBar);
    watcher.setFuture(future);
}

void MainWindow::on_analyze_selected_button_clicked()
{
    ui->result_list->clear();
    analyze_result.clear();
    files_to_analyze.clear();

    analyze_work = true;

    ui->statusbar->showMessage("Анализ...");


    QModelIndexList selectedList = ui->file_list->selectionModel()->selectedIndexes(); // take the list of selected indexes
    std::sort(selectedList.begin(),selectedList.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    for(const QModelIndex& singleIndex : selectedList) {
        files_to_analyze.append(ui->file_list->item(singleIndex.row())->text());
    }

    ui->analyze_selected_button->setEnabled(false);
    ui->analyze_all_button->setEnabled(false);
    ui->set_size_field->setEnabled(false);

    ui->progressBar->show();
    ui->progressBar->setValue(0);

    QFuture<int> future = QtConcurrent::run(analyze,ui->file_list->selectedItems().size(),ui->set_size_field->value(),ui->progressBar);
    watcher.setFuture(future);
}
