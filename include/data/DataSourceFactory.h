#ifndef DATASOURCEFACTORY_H
#define DATASOURCEFACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include "DataSource.h"

// 纯C++数据源工厂
class DataSourceFactory {
public:
    enum class SourceType { CSV, RealTime, Custom };
    
    static DataSourceFactory& getInstance();
    
    std::shared_ptr<DataSource> createCSVSource(const std::string& filename);
    std::shared_ptr<DataSource> createRealTimeSource();
    std::shared_ptr<DataSource> createCustomSource(const std::string& config);

private:
    DataSourceFactory() = default;
};

#endif // DATASOURCEFACTORY_H