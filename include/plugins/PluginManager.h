#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "PluginInterface.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

// 前向声明
class DataModel;

/**
 * @brief 插件管理器类
 * 
 * 负责插件的加载、管理和调度
 */
class PluginManager {
public:
    static PluginManager& getInstance();
    
    // 禁止拷贝和赋值
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    // === 插件管理 ===
    bool loadPlugin(const std::string& name, std::shared_ptr<PluginInterface> plugin);
    bool unloadPlugin(const std::string& name);
    bool reloadPlugin(const std::string& name);
    
    // === 插件查询 ===
    std::shared_ptr<PluginInterface> getPlugin(const std::string& name) const;
    std::vector<std::string> getLoadedPlugins() const;
    std::vector<std::string> getPluginsByType(PluginType type) const;
    bool isPluginLoaded(const std::string& name) const;
    
    // === 数据处理 ===
    bool processData(const std::string& pluginName, 
                    std::shared_ptr<DataModel> input, 
                    std::shared_ptr<DataModel> output);
    
    bool processRealTimeData(const std::string& pluginName, 
                            double input, double& output);
    
    // === 插件链处理 ===
    bool processWithChain(const std::vector<std::string>& pluginChain,
                         std::shared_ptr<DataModel> input,
                         std::shared_ptr<DataModel> output);
    
    // === 配置管理 ===
    bool setPluginParameter(const std::string& pluginName, 
                          const std::string& key, const QVariant& value);
    QVariant getPluginParameter(const std::string& pluginName, 
                              const std::string& key) const;
    
    // === 统计信息 ===
    struct PluginStats {
        std::string name;
        PluginType type;
        bool isLoaded;
        int processingTime;
        size_t processedCount;
        std::string lastError;
    };
    
    PluginStats getPluginStats(const std::string& name) const;
    std::map<std::string, PluginStats> getAllPluginStats() const;

private:
    PluginManager() = default;
    ~PluginManager();
    
    struct PluginInfo {
        std::shared_ptr<PluginInterface> plugin;
        bool isInitialized;
        int totalProcessingTime;
        size_t totalProcessedCount;
        std::string lastError;
    };
    
    std::map<std::string, PluginInfo> m_plugins;
    
    void updatePluginStats(const std::string& name, int processingTime, bool success);
};

#endif // PLUGINMANAGER_H