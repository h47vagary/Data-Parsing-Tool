#include "ApplicationConfig.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// ConfigValue 实现
ApplicationConfig::ConfigValue::ConfigValue() 
    : type(STRING), intValue(0), doubleValue(0.0), boolValue(false) {}

ApplicationConfig::ConfigValue::ConfigValue(const std::string& val) 
    : type(STRING), stringValue(val), intValue(0), doubleValue(0.0), boolValue(false) {}

ApplicationConfig::ConfigValue::ConfigValue(int val) 
    : type(INT), intValue(val), doubleValue(0.0), boolValue(false) {}

ApplicationConfig::ConfigValue::ConfigValue(double val) 
    : type(DOUBLE), doubleValue(val), intValue(0), boolValue(false) {}

ApplicationConfig::ConfigValue::ConfigValue(bool val) 
    : type(BOOL), boolValue(val), intValue(0), doubleValue(0.0) {}

ApplicationConfig::ConfigValue::ConfigValue(const std::vector<std::string>& val) 
    : type(ARRAY), arrayValue(val), intValue(0), doubleValue(0.0), boolValue(false) {}

std::string ApplicationConfig::ConfigValue::toString() const {
    switch (type) {
    case STRING:
        return stringValue;
    case INT:
        return std::to_string(intValue);
    case DOUBLE:
        return std::to_string(doubleValue);
    case BOOL:
        return boolValue ? "true" : "false";
    case ARRAY: {
        std::string result;
        for (size_t i = 0; i < arrayValue.size(); ++i) {
            if (i > 0) result += ",";
            result += arrayValue[i];
        }
        return result;
    }
    default:
        return "";
    }
}

// ApplicationConfig 实现
ApplicationConfig& ApplicationConfig::getInstance() {
    static ApplicationConfig instance;
    return instance;
}

ApplicationConfig::ApplicationConfig() 
    : m_currentGroup(""), m_modified(false) {
    // 设置默认配置
    setDefaultConfig();
}

ApplicationConfig::~ApplicationConfig() {
    // 自动保存修改的配置
    if (m_modified && !m_configFile.empty()) {
        saveToFile(m_configFile);
    }
}

void ApplicationConfig::setDefaultConfig() {
    // 数据源默认配置
    setValue("data_source/type", "csv");
    setValue("data_source/csv_delimiter", ",");
    setValue("data_source/csv_has_header", true);
    setValue("data_source/realtime_sample_rate", 10.0);
    
    // 显示默认配置
    setValue("display/refresh_rate", 30);
    setValue("display/show_grid", true);
    setValue("display/show_legend", true);
    setValue("display/antialiasing", true);
    setValue("display/theme", "default");
    
    // 窗口默认配置
    setValue("window/width", 1200);
    setValue("window/height", 800);
    setValue("window/maximized", false);
    setValue("window/fullscreen", false);
    
    // 插件默认配置
    std::vector<std::string> defaultPlugins;
    defaultPlugins.push_back("filter");
    defaultPlugins.push_back("interpolation");
    setValue("plugins/enabled", defaultPlugins);
    
    // 最近文件列表
    std::vector<std::string> recentFiles;
    setValue("files/recent", recentFiles);
    
    m_modified = false;
}

bool ApplicationConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    m_configFile = filename;
    return loadFromString(buffer.str());
}

bool ApplicationConfig::loadFromString(const std::string& configData) {
    // 简化实现：使用键值对格式
    // 实际项目中可以使用JSON或INI解析库
    std::istringstream ss(configData);
    std::string line;
    
    clear();
    
    while (std::getline(ss, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 解析键值对：key = value
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // 去除首尾空白
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        if (!key.empty() && !value.empty()) {
            // 简单类型检测
            if (value == "true" || value == "false") {
                setValue(key, value == "true");
            } else if (value.find(',') != std::string::npos) {
                // 数组类型
                std::vector<std::string> array = splitString(value, ',');
                setValue(key, array);
            } else if (value.find('.') != std::string::npos) {
                // 尝试解析为double
                try {
                    double dval = std::stod(value);
                    setValue(key, dval);
                } catch (...) {
                    setValue(key, value);
                }
            } else {
                // 尝试解析为int
                try {
                    int ival = std::stoi(value);
                    setValue(key, ival);
                } catch (...) {
                    setValue(key, value);
                }
            }
        }
    }
    
    m_modified = false;
    return true;
}

bool ApplicationConfig::saveToFile(const std::string& filename) const {
    std::string saveFilename = filename.empty() ? m_configFile : filename;
    if (saveFilename.empty()) {
        std::cerr << "未指定配置文件名" << std::endl;
        return false;
    }
    
    std::ofstream file(saveFilename.c_str());
    if (!file.is_open()) {
        std::cerr << "无法创建配置文件: " << saveFilename << std::endl;
        return false;
    }
    
    file << "# 数据解析工具配置文件" << std::endl;
    file << "# 生成时间: " << __DATE__ << " " << __TIME__ << std::endl << std::endl;
    
    // 按组排序输出
    std::map<std::string, std::string> sortedConfig;
    for (std::map<std::string, ConfigValue>::const_iterator it = m_configMap.begin();
         it != m_configMap.end(); ++it) {
        sortedConfig[it->first] = it->second.toString();
    }
    
    std::string currentSection;
    for (std::map<std::string, std::string>::const_iterator it = sortedConfig.begin();
         it != sortedConfig.end(); ++it) {
        // 检查是否需要输出新的节标题
        size_t pos = it->first.find_last_of('/');
        std::string section = (pos != std::string::npos) ? it->first.substr(0, pos) : "";
        
        if (section != currentSection) {
            if (!currentSection.empty()) {
                file << std::endl;
            }
            file << "# " << section << " 配置" << std::endl;
            currentSection = section;
        }
        
        file << it->first << " = " << it->second << std::endl;
    }
    
    file.close();
    m_modified = false;
    return true;
}

std::string ApplicationConfig::saveToString() const {
    std::stringstream ss;
    
    // 简化实现，实际可以使用更复杂的格式
    for (std::map<std::string, ConfigValue>::const_iterator it = m_configMap.begin();
         it != m_configMap.end(); ++it) {
        ss << it->first << "=" << it->second.toString() << std::endl;
    }
    
    return ss.str();
}

// 配置值设置
void ApplicationConfig::setValue(const std::string& key, const std::string& value) {
    setValue(key, ConfigValue(value));
}

void ApplicationConfig::setValue(const std::string& key, int value) {
    setValue(key, ConfigValue(value));
}

void ApplicationConfig::setValue(const std::string& key, double value) {
    setValue(key, ConfigValue(value));
}

void ApplicationConfig::setValue(const std::string& key, bool value) {
    setValue(key, ConfigValue(value));
}

void ApplicationConfig::setValue(const std::string& key, const std::vector<std::string>& value) {
    setValue(key, ConfigValue(value));
}

void ApplicationConfig::setValue(const std::string& key, const ConfigValue& value) {
    std::string fullKey = makeFullKey(key);
    ConfigValue oldValue = getValue(fullKey);
    
    m_configMap[fullKey] = value;
    m_modified = true;
    
    // 通知监听器
    if (oldValue.toString() != value.toString()) {
        notifyListeners(fullKey, value);
    }
}

// 配置值获取
ApplicationConfig::ConfigValue ApplicationConfig::getValue(const std::string& key) const {
    std::string fullKey = makeFullKey(key);
    std::map<std::string, ConfigValue>::const_iterator it = m_configMap.find(fullKey);
    if (it != m_configMap.end()) {
        return it->second;
    }
    return ConfigValue(); // 返回默认值
}

std::string ApplicationConfig::getString(const std::string& key, const std::string& defaultValue) const {
    ConfigValue value = getValue(key);
    if (value.type == ConfigValue::STRING) {
        return value.stringValue;
    }
    return defaultValue;
}

int ApplicationConfig::getInt(const std::string& key, int defaultValue) const {
    ConfigValue value = getValue(key);
    if (value.type == ConfigValue::INT) {
        return value.intValue;
    }
    return defaultValue;
}

double ApplicationConfig::getDouble(const std::string& key, double defaultValue) const {
    ConfigValue value = getValue(key);
    if (value.type == ConfigValue::DOUBLE) {
        return value.doubleValue;
    }
    return defaultValue;
}

bool ApplicationConfig::getBool(const std::string& key, bool defaultValue) const {
    ConfigValue value = getValue(key);
    if (value.type == ConfigValue::BOOL) {
        return value.boolValue;
    }
    return defaultValue;
}

std::vector<std::string> ApplicationConfig::getArray(const std::string& key) const {
    ConfigValue value = getValue(key);
    if (value.type == ConfigValue::ARRAY) {
        return value.arrayValue;
    }
    return std::vector<std::string>();
}

bool ApplicationConfig::hasKey(const std::string& key) const {
    std::string fullKey = makeFullKey(key);
    return m_configMap.find(fullKey) != m_configMap.end();
}

// 配置组操作
void ApplicationConfig::beginGroup(const std::string& prefix) {
    if (m_currentGroup.empty()) {
        m_currentGroup = prefix;
    } else {
        m_currentGroup += "/" + prefix;
    }
}

void ApplicationConfig::endGroup() {
    size_t pos = m_currentGroup.find_last_of('/');
    if (pos != std::string::npos) {
        m_currentGroup = m_currentGroup.substr(0, pos);
    } else {
        m_currentGroup.clear();
    }
}

std::string ApplicationConfig::currentGroup() const {
    return m_currentGroup;
}

// 监听器管理
void ApplicationConfig::addListener(const std::string& key, ConfigListener* listener) {
    if (listener) {
        m_listeners[key].push_back(listener);
    }
}

void ApplicationConfig::removeListener(const std::string& key, ConfigListener* listener) {
    std::map<std::string, std::vector<ConfigListener*> >::iterator it = m_listeners.find(key);
    if (it != m_listeners.end()) {
        std::vector<ConfigListener*>& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }
}

void ApplicationConfig::removeListener(ConfigListener* listener) {
    for (std::map<std::string, std::vector<ConfigListener*> >::iterator it = m_listeners.begin();
         it != m_listeners.end(); ++it) {
        std::vector<ConfigListener*>& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }
}

// 配置管理
void ApplicationConfig::clear() {
    m_configMap.clear();
    m_modified = true;
}

bool ApplicationConfig::removeKey(const std::string& key) {
    std::string fullKey = makeFullKey(key);
    std::map<std::string, ConfigValue>::iterator it = m_configMap.find(fullKey);
    if (it != m_configMap.end()) {
        m_configMap.erase(it);
        m_modified = true;
        return true;
    }
    return false;
}

std::vector<std::string> ApplicationConfig::getAllKeys() const {
    std::vector<std::string> keys;
    for (std::map<std::string, ConfigValue>::const_iterator it = m_configMap.begin();
         it != m_configMap.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

std::map<std::string, ApplicationConfig::ConfigValue> ApplicationConfig::getAllValues() const {
    return m_configMap;
}

// 应用特定配置方法
std::string ApplicationConfig::getDataSourceType() const {
    return getString("data_source/type", "csv");
}

void ApplicationConfig::setDataSourceType(const std::string& type) {
    setValue("data_source/type", type);
}

int ApplicationConfig::getPlotRefreshRate() const {
    return getInt("display/refresh_rate", 30);
}

void ApplicationConfig::setPlotRefreshRate(int rate) {
    setValue("display/refresh_rate", rate);
}

bool ApplicationConfig::getShowGrid() const {
    return getBool("display/show_grid", true);
}

void ApplicationConfig::setShowGrid(bool show) {
    setValue("display/show_grid", show);
}

std::vector<std::string> ApplicationConfig::getEnabledPlugins() const {
    return getArray("plugins/enabled");
}

void ApplicationConfig::setEnabledPlugins(const std::vector<std::string>& plugins) {
    setValue("plugins/enabled", plugins);
}

int ApplicationConfig::getWindowWidth() const {
    return getInt("window/width", 1200);
}

int ApplicationConfig::getWindowHeight() const {
    return getInt("window/height", 800);
}

void ApplicationConfig::setWindowSize(int width, int height) {
    setValue("window/width", width);
    setValue("window/height", height);
}

// 私有方法
std::string ApplicationConfig::makeFullKey(const std::string& key) const {
    if (m_currentGroup.empty()) {
        return key;
    }
    return m_currentGroup + "/" + key;
}

void ApplicationConfig::notifyListeners(const std::string& key, const ConfigValue& newValue) {
    std::map<std::string, std::vector<ConfigListener*> >::iterator it = m_listeners.find(key);
    if (it != m_listeners.end()) {
        const std::vector<ConfigListener*>& listeners = it->second;
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->onConfigChanged(key, newValue);
        }
    }
}

std::string ApplicationConfig::escapeString(const std::string& str) {
    // 简化实现
    return str;
}

std::string ApplicationConfig::unescapeString(const std::string& str) {
    // 简化实现
    return str;
}

std::vector<std::string> ApplicationConfig::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        // 去除首尾空白
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}