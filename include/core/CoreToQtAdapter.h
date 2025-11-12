#ifndef CORETOQTADAPTER_H
#define CORETOQTADAPTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <vector>
#include <memory>

// 前向声明纯C++类
class DataSource;
class PluginInterface;

class CoreToQtAdapter : public QObject {
    Q_OBJECT

public:
    explicit CoreToQtAdapter(QObject* parent = nullptr);
    ~CoreToQtAdapter();
    
    void setDataSource(std::shared_ptr<DataSource> dataSource);
    void setPlugin(std::shared_ptr<PluginInterface> plugin);
    
    QVector<double> getData() const;
    QVector<QPair<double, double>> getDataPairs() const;
    void processData();

signals:
    void dataReady(const QVector<double>& data);
    void dataProcessed(const QVector<double>& result);
    void errorOccurred(const QString& error);

public slots:
    void loadCSVFile(const QString& filename);
    void startRealTimeData();
    void stopRealTimeData();
    void applyFilter(const QString& filterType);

private slots:
    void handleDataReady();
    void handleError(const std::string& error);

private:
    std::shared_ptr<DataSource> m_dataSource;
    std::shared_ptr<PluginInterface> m_currentPlugin;
    std::vector<double> m_currentData;
};

#endif // CORETOQTADAPTER_H