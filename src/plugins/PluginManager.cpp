#include "PluginManager.h"
#include "FilterPlugin.h"
#include "InterpolationPlugin.h"
#include "ExportPlugin.h"
#include <algorithm>
#include <chrono>

PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

PluginManager::~PluginManager() {
    // 关闭所有插件
    for (auto& pair : m_plugins) {
        if (pair.second.plugin) {
            pair.second.plugin->shutdown();
        }
    }
    m_plugins.clear();
}

bool PluginManager::loadPlugin(const std::string& name, std::shared_ptr<PluginInterface> plugin) {
    if (!plugin) {
        return false;
    }
    
    if (m_plugins.find(name) != m_plugins.end()) {
        // 插件已存在，先卸载
        unloadPlugin(name);
    }
    
    // 初始化插件
    if (!plugin->initialize()) {
        return false;
    }
    
    PluginInfo info;
    info.plugin = plugin;
    info.isInitialized = true;
    info.totalProcessingTime = 0;
    info.totalProcessedCount = 0;
    
    m_plugins[name] = info;
    return true;
}

bool PluginManager::unloadPlugin(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return false;
    }
    
    if (it->second.plugin) {
        it->second.plugin->shutdown();
    }
    
    m_plugins.erase(it);
    return true;
}

bool PluginManager::reloadPlugin(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return false;
    }
    
    auto plugin = it->second.plugin;
    if (!plugin) {
        return false;
    }
    
    // 先关闭再重新初始化
    plugin->shutdown();
    if (plugin->initialize()) {
        it->second.isInitialized = true;
        return true;
    }
    
    return false;
}

std::shared_ptr<PluginInterface> PluginManager::getPlugin(const std::string& name) const {
    auto it = m_plugins.find(name);
    if (it != m_plugins.end() && it->second.isInitialized) {
        return it->second.plugin;
    }
    return nullptr;
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::vector<std::string> plugins;
    for (const auto& pair : m_plugins) {
        if (pair.second.isInitialized) {
            plugins.push_back(pair.first);
        }
    }
    return plugins;
}

std::vector<std::string> PluginManager::getPluginsByType(PluginType type) const {
    std::vector<std::string> plugins;
    for (const auto& pair : m_plugins) {
        if (pair.second.isInitialized && pair.second.plugin->getType() == type) {
            plugins.push_back(pair.first);
        }
    }
    return plugins;
}

bool PluginManager::isPluginLoaded(const std::string& name) const {
    auto it = m_plugins.find(name);
    return it != m_plugins.end() && it->second.isInitialized;
}

bool PluginManager::processData(const std::string& pluginName, 
                               std::shared_ptr<DataModel> input, 
                               std::shared_ptr<DataModel> output) {
    auto plugin = getPlugin(pluginName);
    if (!plugin || !input || !output) {
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    bool success = plugin->processData(input, output);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    int processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    updatePluginStats(pluginName, processingTime, success);
    return success;
}

bool PluginManager::processRealTimeData(const std::string& pluginName, 
                                       double input, double& output) {
    auto plugin = getPlugin(pluginName);
    if (!plugin) {
        return false;
    }
    
    // 检查是否支持实时处理
    auto realTimePlugin = std::dynamic_pointer_cast<RealTimePluginInterface>(plugin);
    if (!realTimePlugin) {
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    output = realTimePlugin->processRealTime(input);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    int processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    updatePluginStats(pluginName, processingTime, true);
    return true;
}

bool PluginManager::processWithChain(const std::vector<std::string>& pluginChain,
                                   std::shared_ptr<DataModel> input,
                                   std::shared_ptr<DataModel> output) {
    if (pluginChain.empty() || !input) {
        return false;
    }
    
    std::shared_ptr<DataModel> currentInput = input;
    std::shared_ptr<DataModel> tempOutput;
    
    for (size_t i = 0; i < pluginChain.size(); ++i) {
        const auto& pluginName = pluginChain[i];
        bool isLast = (i == pluginChain.size() - 1);
        
        if (isLast) {
            // 最后一个插件，输出到最终结果
            if (!processData(pluginName, currentInput, output)) {
                return false;
            }
        } else {
            // 中间插件，使用临时输出
            tempOutput = std::make_shared<DataModel>();
            if (!processData(pluginName, currentInput, tempOutput)) {
                return false;
            }
            currentInput = tempOutput;
        }
    }
    
    return true;
}

bool PluginManager::setPluginParameter(const std::string& pluginName, 
                                     const std::string& key, const QVariant& value) {
    auto plugin = getPlugin(pluginName);
    if (!plugin) {
        return false;
    }
    
    return plugin->setParameter(key, value);
}

QVariant PluginManager::getPluginParameter(const std::string& pluginName, 
                                         const std::string& key) const {
    auto plugin = getPlugin(pluginName);
    if (!plugin) {
        return QVariant();
    }
    
    return plugin->getParameter(key);
}

PluginManager::PluginStats PluginManager::getPluginStats(const std::string& name) const {
    PluginStats stats;
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return stats;
    }
    
    stats.name = name;
    stats.isLoaded = it->second.isInitialized;
    stats.processingTime = it->second.totalProcessingTime;
    stats.processedCount = it->second.totalProcessedCount;
    stats.lastError = it->second.lastError;
    
    if (it->second.plugin) {
        stats.type = it->second.plugin->getType();
    }
    
    return stats;
}

std::map<std::string, PluginManager::PluginStats> PluginManager::getAllPluginStats() const {
    std::map<std::string, PluginStats> stats;
    for (const auto& pair : m_plugins) {
        stats[pair.first] = getPluginStats(pair.first);
    }
    return stats;
}

void PluginManager::updatePluginStats(const std::string& name, int processingTime, bool success) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return;
    }
    
    it->second.totalProcessingTime += processingTime;
    it->second.totalProcessedCount++;
    
    if (!success && it->second.plugin) {
        it->second.lastError = it->second.plugin->getLastError();
    } else if (success) {
        it->second.lastError.clear();
    }
}