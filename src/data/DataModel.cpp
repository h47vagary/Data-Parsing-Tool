#include "DataModel.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iostream>

// 静态成员初始化
DataModel::DataSeries DataModel::s_emptySeries;

DataModel::DataModel() : m_pointCount(0) {}

void DataModel::addField(const std::string& fieldName) {
    if (m_dataSeries.find(fieldName) == m_dataSeries.end()) {
        m_dataSeries[fieldName] = DataSeries();
        // 为新字段初始化元数据
        m_fieldMetadata[fieldName]["color"] = MetadataValue("auto");
        m_fieldMetadata[fieldName]["visible"] = MetadataValue(true);
    }
}

void DataModel::removeField(const std::string& fieldName) {
    m_dataSeries.erase(fieldName);
    m_fieldMetadata.erase(fieldName);
}

bool DataModel::hasField(const std::string& fieldName) const {
    return m_dataSeries.find(fieldName) != m_dataSeries.end();
}

std::vector<std::string> DataModel::getFieldNames() const {
    std::vector<std::string> names;
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

void DataModel::clear() {
    for (std::map<std::string, DataSeries>::iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        it->second.clear();
    }
    m_pointCount = 0;
}

void DataModel::clearField(const std::string& fieldName) {
    if (hasField(fieldName)) {
        m_dataSeries[fieldName].clear();
        // 重新计算点数
        size_t maxSize = 0;
        for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
             it != m_dataSeries.end(); ++it) {
            maxSize = std::max(maxSize, it->second.size());
        }
        m_pointCount = maxSize;
    }
}

void DataModel::addDataPoint(const std::map<std::string, double>& pointData) {
    for (std::map<std::string, double>::const_iterator it = pointData.begin(); 
         it != pointData.end(); ++it) {
        const std::string& fieldName = it->first;
        double value = it->second;
        
        // 如果字段不存在，自动创建
        if (!hasField(fieldName)) {
            addField(fieldName);
        }
        
        m_dataSeries[fieldName].push_back(value);
    }
    
    // 更新点数
    size_t maxSize = 0;
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        maxSize = std::max(maxSize, it->second.size());
    }
    m_pointCount = maxSize;
}

void DataModel::addDataSeries(const std::string& fieldName, const DataSeries& data) {
    if (!hasField(fieldName)) {
        addField(fieldName);
    }
    
    m_dataSeries[fieldName] = data;
    m_pointCount = std::max(m_pointCount, data.size());
}

void DataModel::addDataPoints(const std::vector<std::map<std::string, double> >& points) {
    for (size_t i = 0; i < points.size(); ++i) {
        addDataPoint(points[i]);
    }
}

const DataModel::DataSeries& DataModel::getDataSeries(const std::string& fieldName) const {
    std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.find(fieldName);
    if (it != m_dataSeries.end()) {
        return it->second;
    }
    return s_emptySeries;
}

double DataModel::getValue(const std::string& fieldName, size_t index) const {
    std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.find(fieldName);
    if (it == m_dataSeries.end() || index >= it->second.size()) {
        return 0.0;
    }
    return it->second[index];
}

bool DataModel::getDataPoint(size_t index, std::map<std::string, double>& point) const {
    if (index >= m_pointCount) {
        return false;
    }
    
    point.clear();
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        const std::string& fieldName = it->first;
        const DataSeries& series = it->second;
        
        if (index < series.size()) {
            point[fieldName] = series[index];
        } else {
            point[fieldName] = 0.0; // 默认值
        }
    }
    
    return true;
}

void DataModel::setFieldMetadata(const std::string& fieldName, const std::string& key, 
                                const MetadataValue& value) {
    if (hasField(fieldName)) {
        m_fieldMetadata[fieldName][key] = value;
    }
}

DataModel::MetadataValue DataModel::getFieldMetadata(const std::string& fieldName, 
                                                   const std::string& key) const {
    std::map<std::string, std::map<std::string, MetadataValue> >::const_iterator fieldIt = 
        m_fieldMetadata.find(fieldName);
    if (fieldIt != m_fieldMetadata.end()) {
        std::map<std::string, MetadataValue>::const_iterator metaIt = fieldIt->second.find(key);
        if (metaIt != fieldIt->second.end()) {
            return metaIt->second;
        }
    }
    return MetadataValue(); // 返回默认值
}

void DataModel::setFieldColor(const std::string& fieldName, const std::string& color) {
    setFieldMetadata(fieldName, "color", MetadataValue(color));
}

void DataModel::setFieldVisible(const std::string& fieldName, bool visible) {
    setFieldMetadata(fieldName, "visible", MetadataValue(visible));
}

bool DataModel::isValid() const {
    return !empty() && checkConsistency();
}

size_t DataModel::size() const {
    return m_pointCount;
}

bool DataModel::empty() const {
    return m_pointCount == 0;
}

bool DataModel::checkConsistency() const {
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        if (it->second.size() != m_pointCount && !it->second.empty()) {
            return false;
        }
    }
    return true;
}

DataModel::Statistics DataModel::calculateStatistics() const {
    Statistics stats;
    stats.totalPoints = m_pointCount;
    stats.validPoints = 0;
    
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        const std::string& fieldName = it->first;
        const DataSeries& series = it->second;
        
        if (series.empty()) {
            continue;
        }
        
        // 计算范围
        double minVal = series[0];
        double maxVal = series[0];
        double sum = 0.0;
        
        for (size_t i = 0; i < series.size(); ++i) {
            minVal = std::min(minVal, series[i]);
            maxVal = std::max(maxVal, series[i]);
            sum += series[i];
        }
        
        stats.ranges[fieldName] = std::make_pair(minVal, maxVal);
        stats.averages[fieldName] = sum / series.size();
        stats.validPoints = std::max(stats.validPoints, series.size());
    }
    
    return stats;
}

std::shared_ptr<DataModel> DataModel::getSubset(size_t startIndex, size_t endIndex) const {
    std::shared_ptr<DataModel> subset(new DataModel());
    
    if (startIndex >= m_pointCount || endIndex > m_pointCount || startIndex >= endIndex) {
        return subset;
    }
    
    for (std::map<std::string, DataSeries>::const_iterator it = m_dataSeries.begin(); 
         it != m_dataSeries.end(); ++it) {
        const std::string& fieldName = it->first;
        const DataSeries& series = it->second;
        
        if (series.size() > startIndex) {
            size_t actualEnd = std::min(endIndex, series.size());
            DataSeries subSeries(series.begin() + startIndex, series.begin() + actualEnd);
            subset->addDataSeries(fieldName, subSeries);
        }
    }
    
    return subset;
}

std::shared_ptr<DataModel> DataModel::getSubsetByFields(const std::vector<std::string>& fieldNames) const {
    std::shared_ptr<DataModel> subset(new DataModel());
    
    for (size_t i = 0; i < fieldNames.size(); ++i) {
        const std::string& fieldName = fieldNames[i];
        if (hasField(fieldName)) {
            subset->addDataSeries(fieldName, getDataSeries(fieldName));
        }
    }
    
    return subset;
}