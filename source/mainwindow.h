#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QFutureWatcher>
#include <QMap>
#include <QList>
#include <QProgressBar>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_load_file_button_clicked();

    void on_file_list_itemClicked(QListWidgetItem *item);

    void on_remove_selected_button_clicked();

    void on_analyze_all_button_clicked();

    void finished_analyze();

    void on_analyze_selected_button_clicked();

private:
    Ui::MainWindow *ui;

    static int analyze(int items_number, int set_size, QProgressBar *pb);

    QFutureWatcher<int> watcher;

    static QList<QString> files_to_analyze;
    static QMap<QString, int> analyze_result;
    bool analyze_work;
};
#endif // MAINWINDOW_H
