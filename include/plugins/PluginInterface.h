#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <vector>
#include <string>
#include <memory>

// 纯C++插件接口
class PluginInterface {
public:
    virtual ~PluginInterface() = default;
    
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    
    virtual bool initialize() = 0;
    virtual std::vector<double> processData(const std::vector<double>& input) = 0;
    
    virtual bool supportsRealTime() const { return false; }
    virtual double processRealTime(double input) { return input; }
};

// 插件管理器
class PluginManager {
public:
    static PluginManager& getInstance();
    
    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& name);
    std::shared_ptr<PluginInterface> getPlugin(const std::string& name);
    
    std::vector<std::string> getLoadedPlugins() const;

private:
    PluginManager() = default;
    std::map<std::string, std::shared_ptr<PluginInterface>> m_plugins;
};

#endif // PLUGININTERFACE_H 