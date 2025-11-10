#include "main_window.h"
#include "ui_main_window.h"
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QMetaType>
#include <QFile>
#include "csv_parser.h"
#include <iostream>
#include <memory>


MainWindow::MainWindow(QWidget  *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , last_open_dir_("")
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::pressed, this, &MainWindow::slot_button_press);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_button_press()
{
    qDebug() << __FUNCTION__ << endl;

    QString filename = QFileDialog::getOpenFileName(this, tr("选择CSV文件"), last_open_dir_.isEmpty() ? "" : last_open_dir_, tr("CSV Files (*.csv);All Files (*.*)"));
    if (!filename.isEmpty())
    {
        QFileInfo fileInfo(filename);
        last_open_dir_ = fileInfo.absolutePath();
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.completeSuffix();
        QString dir = fileInfo.absolutePath();
        QString filename_filter = dir + "/" + baseName + "_filtered." + suffix;

        CSVParserWindow* csv_parser_window = new CSVParserWindow();
        csv_parser_window->setAttribute(Qt::WA_DeleteOnClose);
        csv_parser_window->loadData(filename.toStdString(), false);
        csv_parser_window->save_data_to_file(filename_filter.toStdString());
        csv_parser_window->plotData();
        csv_parser_window->show();
    }
}