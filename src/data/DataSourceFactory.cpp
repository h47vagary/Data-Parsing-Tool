#include "DataSourceFactory.h"
#include "CSVDataSource.h"
#include <memory>

DataSourceFactory& DataSourceFactory::getInstance() {
    static DataSourceFactory instance;
    return instance;
}

std::shared_ptr<DataSource> DataSourceFactory::createCSVSource(const std::string& filename) {
    auto source = std::make_shared<CSVDataSource>();
    if (source->initialize(filename)) {
        return source;
    }
    return nullptr;
}

std::shared_ptr<DataSource> DataSourceFactory::createRealTimeSource() {
    // 实现实时数据源
    return nullptr;
}

std::shared_ptr<DataSource> DataSourceFactory::createCustomSource(const std::string& config) {
    // 实现自定义数据源
    return nullptr;
}