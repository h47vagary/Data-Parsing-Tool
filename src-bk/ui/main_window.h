#pragma once

#include <QMainWindow>
#include <QList>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QVector>
#include <QTimer>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget  *parent = nullptr);
    ~MainWindow();

private slots:
    void slot_button_press();
public slots:

signals:

private:
    Ui::MainWindow *ui;

private:
    QString last_open_dir_;


public:

};