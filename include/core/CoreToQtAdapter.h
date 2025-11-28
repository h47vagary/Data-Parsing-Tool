#ifndef CORETOQTADAPTER_H
#define CORETOQTADAPTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <memory>

// 前向声明纯C++类
class DataSource;
class DataModel;
class DataSourceFactory;
class PluginManager;
class PluginInterface;

/**
 * @brief 核心层到QT层的适配器类
 * 
 * 功能：
 * - 将纯C++数据源适配为QT信号槽接口
 * - 管理数据源生命周期
 * - 提供类型转换（C++类型 -> QT类型）
 * - 处理线程间通信
 */
class CoreToQtAdapter : public QObject {
    Q_OBJECT

public:
    explicit CoreToQtAdapter(QObject* parent = nullptr);
    ~CoreToQtAdapter();
    
    // === 数据源管理 ===
    bool loadCSVFile(const QString& filename);
    bool loadCustomData(const QString& filename, const QMap<QString, QVariant>& config);
    bool startRealTimeData(const QMap<QString, QVariant>& config);
    void stopRealTimeData();
    
    // === 插件管理 ===
    bool loadPlugin(const QString& pluginPath);
    bool unloadPlugin(const QString& pluginName);
    QStringList getLoadedPlugins() const;
    bool applyPlugin(const QString& pluginName, const QMap<QString, QVariant>& parameters);
    
    // === 数据访问 ===
    QVector<double> getDataSeries(const QString& fieldName) const;
    QVector<QPair<double, double>> getDataPairs(const QString& xField, const QString& yField) const;
    QVariantMap getDataPoint(int index) const;
    QStringList getFieldNames() const;
    
    // === 数据操作 ===
    bool saveDataToFile(const QString& filename);
    bool exportData(const QString& filename, const QString& format);
    void clearData();
    
    // === 配置管理 ===
    void setDataSourceConfig(const QMap<QString, QVariant>& config);
    void setPluginConfig(const QString& pluginName, const QMap<QString, QVariant>& config);
    
    // === 状态查询 ===
    bool isDataSourceReady() const;
    bool hasData() const;
    int getDataPointCount() const;
    QString getCurrentDataSourceType() const;

public slots:
    // === 数据源控制 ===
    void onLoadDataRequested(const QString& filename, const QString& dataType);
    void onStartRealTimeRequested(const QMap<QString, QVariant>& config);
    void onStopRealTimeRequested();
    void onClearDataRequested();
    
    // === 数据处理 ===
    void onApplyFilterRequested(const QString& filterType, const QMap<QString, QVariant>& parameters);
    void onApplyInterpolationRequested(const QMap<QString, QVariant>& parameters);
    void onExportDataRequested(const QString& filename, const QString& format);
    
    // === 数据查询 ===
    void onGetStatisticsRequested();
    void onGetFieldInfoRequested();

signals:
    // === 数据状态信号 ===
    void dataLoaded(bool success, const QString& message);
    void dataLoadProgress(int progress, const QString& status);
    void dataCleared();
    
    // === 数据更新信号 ===
    void dataUpdated();
    void realTimeDataPointAdded(const QVariantMap& dataPoint);
    void dataProcessed(const QString& operation, bool success);
    
    // === 错误信号 ===
    void errorOccurred(const QString& errorMessage, int errorCode);
    void warningOccurred(const QString& warningMessage);
    
    // === 状态信号 ===
    void dataSourceStatusChanged(const QString& status);
    void pluginStatusChanged(const QString& pluginName, bool loaded);
    
    // === 统计信息信号 ===
    void statisticsReady(const QVariantMap& statistics);
    void fieldInfoReady(const QVariantMap& fieldInfo);

private slots:
    // 处理核心层回调
    void handleCoreDataReady();
    void handleCoreError(const std::string& errorMessage);
    void handleCoreRealTimeData(double value);

private:
    // 初始化方法
    void initializeCoreComponents();
    void setupConnections();
    
    // 数据转换方法
    QVector<double> convertToQVector(const std::vector<double>& vec) const;
    QVariantMap convertDataPointToVariantMap(const std::map<std::string, double>& point) const;
    QVariantMap convertStatisticsToVariantMap() const;
    
    // 工具方法
    std::string qstringToString(const QString& qstr) const;
    QString stringToQString(const std::string& str) const;
    QMap<QString, QVariant> convertConfig(const std::map<std::string, std::string>& config) const;
    
    // 核心组件
    std::shared_ptr<DataSource> m_currentDataSource;
    std::shared_ptr<DataModel> m_currentDataModel;
    std::shared_ptr<DataSourceFactory> m_dataSourceFactory;
    std::shared_ptr<PluginManager> m_pluginManager;
    
    // 状态管理
    QString m_currentDataSourceType;
    bool m_isRealTimeRunning;
    
    // 配置管理
    QMap<QString, QVariant> m_dataSourceConfig;
    QMap<QString, QMap<QString, QVariant>> m_pluginConfigs;
    
    // 防止重复包含头文件
    class CoreComponentHolder;
    std::unique_ptr<CoreComponentHolder> m_coreComponents;
};

#endif // CORETOQTADAPTER_H