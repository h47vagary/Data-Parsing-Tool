#include "MainWindow.h"
#include "CoreToQtAdapter.h"
#include "visualization/PlotWidget.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent) 
    : QMainWindow(parent)
    , m_plotWidget(new PlotWidget(this))
    , m_adapter(new CoreToQtAdapter(this))
{
    setupUI();
    setupConnections();
    initializeCoreComponents();
}

void MainWindow::setupConnections() {
    // 连接适配器信号
    connect(m_adapter, &CoreToQtAdapter::dataReady, 
            this, &MainWindow::onDataReady);
    connect(m_adapter, &CoreToQtAdapter::errorOccurred,
            this, &MainWindow::onErrorOccurred);
}

void MainWindow::onOpenFile() {
    QString filename = QFileDialog::getOpenFileName(
        this, "打开数据文件", "", "CSV文件 (*.csv);;所有文件 (*)");
    
    if (!filename.isEmpty()) {
        m_adapter->loadCSVFile(filename);
    }
}

void MainWindow::onDataReady(const QVector<double>& data) {
    // 将数据传递给绘图控件
    QVector<double> xData;
    for (int i = 0; i < data.size(); ++i) {
        xData.append(i);
    }
    m_plotWidget->addDataset("数据", xData, data);
}