#ifndef CORETOQTADAPTER_H
#define CORETOQTADAPTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

// 前向声明
class DataSource;
class DataModel;

class CoreToQtAdapter : public QObject {
    Q_OBJECT

public:
    explicit CoreToQtAdapter(QObject* parent = nullptr);
    ~CoreToQtAdapter();
    
    void setDataSource(std::shared_ptr<DataSource> dataSource);
    std::shared_ptr<DataModel> getCurrentData() const { return m_currentData; }
    
    QVector<double> getDataAsQVector() const;
    QVector<QPair<double, double>> getDataPairs() const;

public slots:
    void loadCSVFile(const QString& filename);
    void startRealTimeData();
    void stopRealTimeData();

signals:
    void dataReady(std::shared_ptr<DataModel> data);
    void errorOccurred(const QString& error);

private slots:
    void handleDataReady();
    void handleError(const std::string& error);

private:
    std::shared_ptr<DataSource> m_dataSource;
    std::shared_ptr<DataModel> m_currentData;
};

#endif // CORETOQTADAPTER_H