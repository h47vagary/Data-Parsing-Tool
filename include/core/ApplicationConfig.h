#ifndef APPLICATIONCONFIG_H
#define APPLICATIONCONFIG_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>

/**
 * @brief 应用程序配置管理类
 * 
 * 功能特性：
 * - 支持键值对配置存储
 * - 支持配置文件读写（JSON/INI格式）
 * - 支持配置分组
 * - 支持配置变更通知
 * - 线程安全
 */
class ApplicationConfig {
public:
    // 配置值类型
    struct ConfigValue {
        enum ValueType { STRING, INT, DOUBLE, BOOL, ARRAY };
        ValueType type;
        std::string stringValue;
        int intValue;
        double doubleValue;
        bool boolValue;
        std::vector<std::string> arrayValue;
        
        ConfigValue();
        ConfigValue(const std::string& val);
        ConfigValue(int val);
        ConfigValue(double val);
        ConfigValue(bool val);
        ConfigValue(const std::vector<std::string>& val);
        
        std::string toString() const;
    };

    // 配置变更监听器接口
    class ConfigListener {
    public:
        virtual ~ConfigListener() = default;
        virtual void onConfigChanged(const std::string& key, const ConfigValue& newValue) = 0;
    };

    static ApplicationConfig& getInstance();
    
    // 禁止拷贝和赋值
    ApplicationConfig(const ApplicationConfig&) = delete;
    ApplicationConfig& operator=(const ApplicationConfig&) = delete;
    
    // === 配置文件操作 ===
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename = "") const;
    bool loadFromString(const std::string& configData);
    std::string saveToString() const;
    
    // === 配置值访问 ===
    // 设置配置值
    void setValue(const std::string& key, const std::string& value);
    void setValue(const std::string& key, int value);
    void setValue(const std::string& key, double value);
    void setValue(const std::string& key, bool value);
    void setValue(const std::string& key, const std::vector<std::string>& value);
    void setValue(const std::string& key, const ConfigValue& value);
    
    // 获取配置值
    ConfigValue getValue(const std::string& key) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    std::vector<std::string> getArray(const std::string& key) const;
    
    // 检查配置是否存在
    bool hasKey(const std::string& key) const;
    
    // === 配置组操作 ===
    void beginGroup(const std::string& prefix);
    void endGroup();
    std::string currentGroup() const;
    
    // === 监听器管理 ===
    void addListener(const std::string& key, ConfigListener* listener);
    void removeListener(const std::string& key, ConfigListener* listener);
    void removeListener(ConfigListener* listener);
    
    // === 配置管理 ===
    void clear();
    bool removeKey(const std::string& key);
    std::vector<std::string> getAllKeys() const;
    std::map<std::string, ConfigValue> getAllValues() const;
    
    // === 应用特定配置 ===
    // 数据源配置
    std::string getDataSourceType() const;
    void setDataSourceType(const std::string& type);
    
    // 显示配置
    int getPlotRefreshRate() const;
    void setPlotRefreshRate(int rate);
    bool getShowGrid() const;
    void setShowGrid(bool show);
    
    // 插件配置
    std::vector<std::string> getEnabledPlugins() const;
    void setEnabledPlugins(const std::vector<std::string>& plugins);
    
    // 窗口配置
    int getWindowWidth() const;
    int getWindowHeight() const;
    void setWindowSize(int width, int height);

private:
    ApplicationConfig(); // 私有构造函数，单例模式
    ~ApplicationConfig();
    
    void setDefaultConfig();

    std::string makeFullKey(const std::string& key) const;
    void notifyListeners(const std::string& key, const ConfigValue& newValue);
    
    static std::string escapeString(const std::string& str);
    static std::string unescapeString(const std::string& str);
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    
    std::map<std::string, ConfigValue> m_configMap;
    std::string m_currentGroup;
    std::string m_configFile;
    
    // 监听器管理：key -> 监听器列表
    std::map<std::string, std::vector<ConfigListener*> > m_listeners;
    
    mutable bool m_modified; // 配置是否被修改
};

#endif // APPLICATIONCONFIG_H