#ifndef DATASOURCEFACTORY_H
#define DATASOURCEFACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

// 前置声明，避免包含QT头文件
class DataSource;

// 纯C++数据源工厂
class DataSourceFactory {
public:
    enum class SourceType { CSV, RealTime, Custom };
    
    static DataSourceFactory& getInstance();
    
    std::shared_ptr<DataSource> createCSVSource(const std::string& filename);
    std::shared_ptr<DataSource> createRealTimeSource();
    std::shared_ptr<DataSource> createCustomSource(const std::string& config);
    
    void registerParser(const std::string& format, std::function<std::vector<double>(const std::string&)> parser);

private:
    DataSourceFactory() = default;
    std::map<std::string, std::function<std::vector<double>(const std::string&)>> m_parsers;
};

#endif // DATASOURCEFACTORY_H