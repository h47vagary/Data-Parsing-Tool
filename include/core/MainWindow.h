#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

// 前向声明
class PlotWidget;
class CoreToQtAdapter;
class DataSourceFactory;
class PluginManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenFile();
    void onSaveImage();
    void onPluginSelected();
    void onDataReady();
    void onErrorOccurred(const QString& error);

private:
    void setupUI();
    void setupConnections();
    void initializeCoreComponents();
    
    PlotWidget* m_plotWidget;
    CoreToQtAdapter* m_adapter;
    std::shared_ptr<DataSourceFactory> m_dataFactory;
    std::shared_ptr<PluginManager> m_pluginManager;
};

#endif // MAINWINDOW_H