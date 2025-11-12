#ifndef APPLICATIONCONFIG_H
#define APPLICATIONCONFIG_H

#include <string>
#include <unordered_map>
#include <memory>

// 纯C++配置类，不依赖QT
class ApplicationConfig {
public:
    static ApplicationConfig& getInstance();
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename);
    
    void setValue(const std::string& key, const std::string& value);
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    
    int getIntValue(const std::string& key, int defaultValue = 0) const;
    double getDoubleValue(const std::string& key, double defaultValue = 0.0) const;
    bool getBoolValue(const std::string& key, bool defaultValue = false) const;

private:
    ApplicationConfig() = default;
    std::unordered_map<std::string, std::string> m_config;
};

#endif // APPLICATIONCONFIG_H