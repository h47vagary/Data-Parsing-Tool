#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <QVariant>

// 前向声明
class DataModel;

/**
 * @brief 插件类型枚举
 */
enum class PluginType {
    FILTER = 0,         // 数据滤波插件
    INTERPOLATION = 1,   // 数据插值插件
    EXPORT = 2,          // 数据导出插件
    ANALYSIS = 3,        // 数据分析插件
    VISUALIZATION = 4,   // 数据可视化插件
    TRANSFORM = 5,       // 数据变换插件
    VALIDATION = 6       // 数据验证插件
};

/**
 * @brief 插件接口类
 * 
 * 所有插件必须实现的接口
 */
class PluginInterface {
public:
    virtual ~PluginInterface() = default;
    
    // === 插件基本信息 ===
    virtual PluginType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::vector<std::string> getDependencies() const = 0;
    
    // === 插件生命周期 ===
    virtual bool initialize() = 0;
    virtual bool shutdown() = 0;
    virtual bool isInitialized() const = 0;
    
    // === 数据处理 ===
    virtual bool processData(std::shared_ptr<DataModel> input, 
                           std::shared_ptr<DataModel> output) = 0;
    virtual bool supportsRealTime() const = 0;
    virtual bool supportsBatchProcessing() const = 0;
    
    // === 配置管理 ===
    virtual bool setParameter(const std::string& key, const QVariant& value) = 0;
    virtual QVariant getParameter(const std::string& key) const = 0;
    virtual std::map<std::string, QVariant> getDefaultParameters() const = 0;
    virtual bool validateParameters() const = 0;
    
    // === 状态查询 ===
    virtual std::string getLastError() const = 0;
    virtual int getProcessingTime() const = 0; // 处理时间(ms)
    virtual size_t getProcessedCount() const = 0; // 处理的数据点数量
};

/**
 * @brief 实时数据处理插件接口
 */
class RealTimePluginInterface : public PluginInterface {
public:
    virtual ~RealTimePluginInterface() = default;
    
    virtual double processRealTime(double input) = 0;
    virtual void resetRealTimeState() = 0;
};

/**
 * @brief 批量数据处理插件接口
 */
class BatchPluginInterface : public PluginInterface {
public:
    virtual ~BatchPluginInterface() = default;
    
    virtual bool setBatchSize(size_t size) = 0;
    virtual size_t getBatchSize() const = 0;
    virtual bool processBatch(const std::vector<double>& input, 
                             std::vector<double>& output) = 0;
};

#endif // PLUGININTERFACE_H