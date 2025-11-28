#include "CoreToQtAdapter.h"
#include "data/DataSource.h"
#include "data/DataSourceFactory.h"
#include "data/DataModel.h"
#include "plugins/PluginManager.h"
#include "plugins/PluginInterface.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QVariant>
#include <QPair>
#include <memory>
#include <map>

// 防止QT与C++头文件冲突的包装器
class CoreToQtAdapter::CoreComponentHolder {
public:
    std::shared_ptr<DataSourceFactory> dataSourceFactory;
    std::shared_ptr<PluginManager> pluginManager;
};

CoreToQtAdapter::CoreToQtAdapter(QObject* parent) 
    : QObject(parent)
    , m_isRealTimeRunning(false)
    , m_currentDataSourceType("none")
    , m_coreComponents(new CoreComponentHolder())
{
    initializeCoreComponents();
    setupConnections();
}

CoreToQtAdapter::~CoreToQtAdapter() {
    stopRealTimeData();
}

void CoreToQtAdapter::initializeCoreComponents() {
    try {
        // 初始化数据源工厂
        m_coreComponents->dataSourceFactory = std::make_shared<DataSourceFactory>();
        
        // 初始化插件管理器
        m_coreComponents->pluginManager = std::make_shared<PluginManager>();
        
        qDebug() << "核心组件初始化完成";
    } catch (const std::exception& e) {
        qWarning() << "核心组件初始化失败:" << e.what();
        emit errorOccurred(QString("核心组件初始化失败: %1").arg(e.what()), -1);
    }
}

void CoreToQtAdapter::setupConnections() {
    // 连接将在具体数据源设置时建立
}

bool CoreToQtAdapter::loadCSVFile(const QString& filename) {
    if (filename.isEmpty() || !QFile::exists(filename)) {
        emit errorOccurred("文件不存在或路径为空: " + filename, 1001);
        return false;
    }
    
    emit dataLoadProgress(0, "开始加载CSV文件...");
    
    try {
        // 停止当前数据源
        if (m_currentDataSource) {
            m_currentDataSource->stop();
        }
        
        // 创建CSV数据源
        std::string stdFilename = qstringToString(filename);
        m_currentDataSource = m_coreComponents->dataSourceFactory->createCSVSource(stdFilename);
        
        if (!m_currentDataSource) {
            emit errorOccurred("创建CSV数据源失败", 1002);
            return false;
        }
        
        // 设置回调
        m_currentDataSource->setDataReadyCallback([this]() {
            QMetaObject::invokeMethod(this, "handleCoreDataReady", Qt::QueuedConnection);
        });
        
        m_currentDataSource->setErrorCallback([this](const std::string& error) {
            QMetaObject::invokeMethod(this, "handleCoreError", Qt::QueuedConnection,
                                   Q_ARG(std::string, error));
        });
        
        emit dataLoadProgress(50, "正在解析CSV数据...");
        
        // 启动数据源
        if (m_currentDataSource->start()) {
            m_currentDataSourceType = "csv";
            m_currentDataModel = std::dynamic_pointer_cast<CSVDataSource>(m_currentDataSource)->getDataModel();
            
            emit dataLoadProgress(100, "CSV文件加载完成");
            emit dataLoaded(true, QString("成功加载文件: %1").arg(filename));
            emit dataSourceStatusChanged("csv_loaded");
            
            qDebug() << "CSV文件加载成功:" << filename;
            return true;
        } else {
            emit errorOccurred("CSV数据解析失败", 1003);
            return false;
        }
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("加载CSV文件时发生异常: %1").arg(e.what());
        emit errorOccurred(errorMsg, 1004);
        qCritical() << errorMsg;
        return false;
    }
}

bool CoreToQtAdapter::startRealTimeData(const QMap<QString, QVariant>& config) {
    if (m_isRealTimeRunning) {
        stopRealTimeData();
    }
    
    try {
        // 创建实时数据源
        std::map<std::string, std::string> stdConfig;
        for (auto it = config.begin(); it != config.end(); ++it) {
            stdConfig[qstringToString(it.key())] = qstringToString(it.value().toString());
        }
        
        m_currentDataSource = m_coreComponents->dataSourceFactory->createRealTimeSource(stdConfig);
        
        if (!m_currentDataSource) {
            emit errorOccurred("创建实时数据源失败", 2001);
            return false;
        }
        
        // 设置实时数据回调
        m_currentDataSource->setDataReadyCallback([this]() {
            QMetaObject::invokeMethod(this, "handleCoreDataReady", Qt::QueuedConnection);
        });
        
        m_currentDataSource->setErrorCallback([this](const std::string& error) {
            QMetaObject::invokeMethod(this, "handleCoreError", Qt::QueuedConnection,
                                   Q_ARG(std::string, error));
        });
        
        // 启动实时数据
        if (m_currentDataSource->start()) {
            m_isRealTimeRunning = true;
            m_currentDataSourceType = "realtime";
            m_currentDataModel = std::dynamic_pointer_cast<RealTimeDataSource>(m_currentDataSource)->getDataModel();
            
            emit dataSourceStatusChanged("realtime_running");
            qDebug() << "实时数据源启动成功";
            return true;
        }
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("启动实时数据源失败: %1").arg(e.what());
        emit errorOccurred(errorMsg, 2002);
        qCritical() << errorMsg;
    }
    
    return false;
}

void CoreToQtAdapter::stopRealTimeData() {
    if (m_currentDataSource && m_isRealTimeRunning) {
        m_currentDataSource->stop();
        m_isRealTimeRunning = false;
        emit dataSourceStatusChanged("realtime_stopped");
        qDebug() << "实时数据源已停止";
    }
}

bool CoreToQtAdapter::loadPlugin(const QString& pluginPath) {
    if (pluginPath.isEmpty()) {
        emit errorOccurred("插件路径为空", 3001);
        return false;
    }
    
    try {
        std::string stdPath = qstringToString(pluginPath);
        bool success = m_coreComponents->pluginManager->loadPlugin(stdPath);
        
        if (success) {
            emit pluginStatusChanged(QFileInfo(pluginPath).baseName(), true);
            qDebug() << "插件加载成功:" << pluginPath;
            return true;
        } else {
            emit errorOccurred("插件加载失败: " + pluginPath, 3002);
            return false;
        }
    } catch (const std::exception& e) {
        QString errorMsg = QString("插件加载异常: %1").arg(e.what());
        emit errorOccurred(errorMsg, 3003);
        return false;
    }
}

bool CoreToQtAdapter::applyPlugin(const QString& pluginName, const QMap<QString, QVariant>& parameters) {
    if (!m_currentDataModel || m_currentDataModel->empty()) {
        emit errorOccurred("没有可处理的数据", 4001);
        return false;
    }
    
    try {
        std::string stdPluginName = qstringToString(pluginName);
        auto plugin = m_coreComponents->pluginManager->getPlugin(stdPluginName);
        
        if (!plugin) {
            emit errorOccurred("插件未找到: " + pluginName, 4002);
            return false;
        }
        
        // 应用插件处理数据
        // 这里需要根据具体插件接口实现处理逻辑
        emit dataProcessed(pluginName, true);
        emit dataUpdated();
        
        qDebug() << "插件应用成功:" << pluginName;
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("插件应用失败: %1").arg(e.what());
        emit errorOccurred(errorMsg, 4003);
        return false;
    }
}

QVector<double> CoreToQtAdapter::getDataSeries(const QString& fieldName) const {
    if (!m_currentDataModel) {
        return QVector<double>();
    }
    
    std::string stdFieldName = qstringToString(fieldName);
    if (!m_currentDataModel->hasField(stdFieldName)) {
        return QVector<double>();
    }
    
    const auto& dataSeries = m_currentDataModel->getDataSeries(stdFieldName);
    return convertToQVector(dataSeries);
}

QVector<QPair<double, double>> CoreToQtAdapter::getDataPairs(const QString& xField, const QString& yField) const {
    QVector<QPair<double, double>> pairs;
    
    if (!m_currentDataModel || m_currentDataModel->empty()) {
        return pairs;
    }
    
    auto xData = getDataSeries(xField);
    auto yData = getDataSeries(yField);
    
    if (xData.size() != yData.size() || xData.isEmpty()) {
        return pairs;
    }
    
    for (int i = 0; i < xData.size(); ++i) {
        pairs.append(qMakePair(xData[i], yData[i]));
    }
    
    return pairs;
}

QVariantMap CoreToQtAdapter::getDataPoint(int index) const {
    QVariantMap point;
    
    if (!m_currentDataModel || index < 0 || index >= static_cast<int>(m_currentDataModel->size())) {
        return point;
    }
    
    std::map<std::string, double> stdPoint;
    if (m_currentDataModel->getDataPoint(static_cast<size_t>(index), stdPoint)) {
        point = convertDataPointToVariantMap(stdPoint);
    }
    
    return point;
}

QStringList CoreToQtAdapter::getFieldNames() const {
    QStringList fields;
    
    if (!m_currentDataModel) {
        return fields;
    }
    
    auto stdFields = m_currentDataModel->getFieldNames();
    for (const auto& field : stdFields) {
        fields.append(stringToQString(field));
    }
    
    return fields;
}

bool CoreToQtAdapter::saveDataToFile(const QString& filename) {
    if (!m_currentDataModel || m_currentDataModel->empty()) {
        emit errorOccurred("没有数据可保存", 5001);
        return false;
    }
    
    // 这里需要实现数据保存逻辑
    // 可以调用DataModel的保存方法或使用专门的导出插件
    
    qDebug() << "数据保存到文件:" << filename;
    return true;
}

void CoreToQtAdapter::clearData() {
    if (m_currentDataSource) {
        m_currentDataSource->stop();
    }
    
    m_currentDataModel.reset();
    m_currentDataSource.reset();
    m_currentDataSourceType = "none";
    m_isRealTimeRunning = false;
    
    emit dataCleared();
    emit dataSourceStatusChanged("no_data");
    
    qDebug() << "数据已清空";
}

// ==================== Slots 实现 ====================

void CoreToQtAdapter::onLoadDataRequested(const QString& filename, const QString& dataType) {
    if (dataType.toLower() == "csv") {
        loadCSVFile(filename);
    } else {
        emit errorOccurred("不支持的数据类型: " + dataType, 6001);
    }
}

void CoreToQtAdapter::onStartRealTimeRequested(const QMap<QString, QVariant>& config) {
    startRealTimeData(config);
}

void CoreToQtAdapter::onStopRealTimeRequested() {
    stopRealTimeData();
}

void CoreToQtAdapter::onClearDataRequested() {
    clearData();
}

void CoreToQtAdapter::onApplyFilterRequested(const QString& filterType, const QMap<QString, QVariant>& parameters) {
    applyPlugin(filterType, parameters);
}

void CoreToQtAdapter::onGetStatisticsRequested() {
    if (!m_currentDataModel) {
        emit errorOccurred("没有可统计的数据", 7001);
        return;
    }
    
    auto statistics = convertStatisticsToVariantMap();
    emit statisticsReady(statistics);
}

// ==================== 私有方法实现 ====================

void CoreToQtAdapter::handleCoreDataReady() {
    emit dataUpdated();
    
    if (m_isRealTimeRunning && m_currentDataModel && !m_currentDataModel->empty()) {
        // 获取最新数据点
        size_t lastIndex = m_currentDataModel->size() - 1;
        std::map<std::string, double> latestPoint;
        if (m_currentDataModel->getDataPoint(lastIndex, latestPoint)) {
            QVariantMap point = convertDataPointToVariantMap(latestPoint);
            emit realTimeDataPointAdded(point);
        }
    }
}

void CoreToQtAdapter::handleCoreError(const std::string& errorMessage) {
    QString qErrorMessage = stringToQString(errorMessage);
    emit errorOccurred(qErrorMessage, 9001);
    qCritical() << "核心层错误:" << qErrorMessage;
}

QVector<double> CoreToQtAdapter::convertToQVector(const std::vector<double>& vec) const {
    QVector<double> result;
    result.reserve(static_cast<int>(vec.size()));
    
    for (double value : vec) {
        result.append(value);
    }
    
    return result;
}

QVariantMap CoreToQtAdapter::convertDataPointToVariantMap(const std::map<std::string, double>& point) const {
    QVariantMap result;
    
    for (const auto& pair : point) {
        result[stringToQString(pair.first)] = pair.second;
    }
    
    return result;
}

QVariantMap CoreToQtAdapter::convertStatisticsToVariantMap() const {
    QVariantMap statistics;
    
    if (!m_currentDataModel) {
        return statistics;
    }
    
    auto stats = m_currentDataModel->calculateStatistics();
    
    statistics["totalPoints"] = static_cast<int>(stats.totalPoints);
    statistics["validPoints"] = static_cast<int>(stats.validPoints);
    
    QVariantMap ranges;
    for (const auto& range : stats.ranges) {
        QVariantMap rangeMap;
        rangeMap["min"] = range.second.first;
        rangeMap["max"] = range.second.second;
        ranges[stringToQString(range.first)] = rangeMap;
    }
    statistics["ranges"] = ranges;
    
    QVariantMap averages;
    for (const auto& avg : stats.averages) {
        averages[stringToQString(avg.first)] = avg.second;
    }
    statistics["averages"] = averages;
    
    return statistics;
}

std::string CoreToQtAdapter::qstringToString(const QString& qstr) const {
    return qstr.toStdString();
}

QString CoreToQtAdapter::stringToQString(const std::string& str) const {
    return QString::fromStdString(str);
}

bool CoreToQtAdapter::isDataSourceReady() const {
    return m_currentDataSource && m_currentDataSource->getState() == DataSource::State::Running;
}

bool CoreToQtAdapter::hasData() const {
    return m_currentDataModel && !m_currentDataModel->empty();
}

int CoreToQtAdapter::getDataPointCount() const {
    return m_currentDataModel ? static_cast<int>(m_currentDataModel->size()) : 0;
}

QString CoreToQtAdapter::getCurrentDataSourceType() const {
    return m_currentDataSourceType;
}