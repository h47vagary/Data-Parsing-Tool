#include "CustomDataSource.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

CustomDataSource::CustomDataSource() 
    : m_state(State::Stopped)
    , m_hasNewData(false)
    , m_dataModel(std::make_shared<DataModel>())
{
    // 初始化统计信息
    m_stats.totalPoints = 0;
    m_stats.validPoints = 0;
    m_stats.skippedPoints = 0;
}

CustomDataSource::~CustomDataSource() {
    stop();
}

bool CustomDataSource::initialize(const std::string& config) {
    m_sourcePath = config;
    m_state = State::Stopped;
    m_hasNewData = false;
    
    // 重置统计信息
    m_stats.totalPoints = 0;
    m_stats.validPoints = 0;
    m_stats.skippedPoints = 0;
    m_stats.ranges.clear();
    
    return true;
}

bool CustomDataSource::start() {
    if (m_sourcePath.empty()) {
        if (m_errorCallback) {
            m_errorCallback("数据源路径未设置");
        }
        return false;
    }
    
    std::ifstream file(m_sourcePath.c_str());
    if (!file.is_open()) {
        if (m_errorCallback) {
            m_errorCallback("无法打开文件: " + m_sourcePath);
        }
        return false;
    }
    
    // 重置数据模型
    m_dataModel->clear();
    m_stats.totalPoints = 0;
    m_stats.validPoints = 0;
    m_stats.skippedPoints = 0;
    m_stats.ranges.clear();
    
    std::string line;
    int lineNumber = 0;
    int skippedLines = 0;
    
    // 跳过指定行数
    for (int i = 0; i < m_config.skipLines && std::getline(file, line); ++i) {
        skippedLines++;
        lineNumber++;
    }
    
    // 如果有标题行，跳过
    if (m_config.hasHeader && std::getline(file, line)) {
        skippedLines++;
        lineNumber++;
    }
    
    // 解析数据行
    while (std::getline(file, line)) {
        lineNumber++;
        
        // 跳过空行和注释行
        if (line.empty() || line[0] == m_config.commentChar) {
            skippedLines++;
            continue;
        }
        
        std::vector<double> values;
        if (!parseLine(line, values)) {
            m_stats.skippedPoints++;
            continue;
        }
        
        if (values.empty()) {
            m_stats.skippedPoints++;
            continue;
        }
        
        // 创建数据点映射
        std::map<std::string, double> pointData;
        
        // 使用列映射
        for (size_t i = 0; i < values.size(); ++i) {
            std::string fieldName;
            
            // 查找列映射
            std::map<int, std::string>::const_iterator it = m_config.columnMapping.find(i);
            if (it != m_config.columnMapping.end()) {
                fieldName = it->second;
            } else {
                // 如果没有映射，生成默认字段名
                fieldName = "Column_" + std::to_string(i + 1);
            }
            
            // 验证数据
            if (!validateValue(values[i], m_config.validationRule)) {
                m_stats.skippedPoints++;
                break;
            }
            
            pointData[fieldName] = values[i];
        }
        
        if (!pointData.empty()) {
            m_dataModel->addDataPoint(pointData);
            m_stats.validPoints++;
        } else {
            m_stats.skippedPoints++;
        }
    }
    
    m_stats.totalPoints = lineNumber;
    m_stats.skippedPoints += skippedLines;
    
    file.close();
    m_state = State::Running;
    updateDataReady();
    
    std::cout << "自定义数据源加载完成: " << m_stats.validPoints 
              << "/" << m_stats.totalPoints << " 行有效数据" << std::endl;
    
    return true;
}

void CustomDataSource::stop() {
    m_dataModel->clear();
    m_state = State::Stopped;
    m_hasNewData = false;
}

std::vector<double> CustomDataSource::getData() {
    m_hasNewData = false;
    
    // 返回第一个字段的数据作为示例
    std::vector<std::string> fieldNames = m_dataModel->getFieldNames();
    if (!fieldNames.empty()) {
        const DataModel::DataSeries& series = m_dataModel->getDataSeries(fieldNames[0]);
        return std::vector<double>(series.begin(), series.end());
    }
    
    return std::vector<double>();
}

bool CustomDataSource::parseLine(const std::string& line, std::vector<double>& values) {
    if (m_customParser) {
        // 使用自定义解析器
        return m_customParser->parseLine(line, values);
    }
    
    // 使用默认解析逻辑
    std::istringstream ss(line);
    std::string token;
    std::vector<double> parsedValues;
    
    while (std::getline(ss, token, m_config.delimiter)) {
        // 去除首尾空白字符
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        
        if (token.empty()) {
            // 空字段，跳过
            continue;
        }
        
        try {
            double value = std::stod(token);
            parsedValues.push_back(value);
        } catch (const std::exception& e) {
            // 解析失败
            return false;
        }
    }
    
    if (!parsedValues.empty()) {
        values = parsedValues;
        return true;
    }
    
    return false;
}

bool CustomDataSource::validateValue(double value, const ParseConfig::ValidationRule& rule) {
    if (std::isnan(value) && !rule.allowNaN) {
        return false;
    }
    
    if (std::isinf(value) && !rule.allowInfinity) {
        return false;
    }
    
    if (value < rule.minValue || value > rule.maxValue) {
        return false;
    }
    
    return true;
}

void CustomDataSource::setCustomParser(std::unique_ptr<DataParser> parser) {
    m_customParser = std::move(parser);
}

bool CustomDataSource::appendData(const std::vector<std::vector<double>>& newData) {
    if (m_state != State::Running) {
        return false;
    }
    
    for (size_t i = 0; i < newData.size(); ++i) {
        const std::vector<double>& row = newData[i];
        if (row.empty()) {
            continue;
        }
        
        // 创建数据点映射
        std::map<std::string, double> pointData;
        
        for (size_t j = 0; j < row.size(); ++j) {
            std::string fieldName;
            
            // 查找列映射
            std::map<int, std::string>::const_iterator it = m_config.columnMapping.find(j);
            if (it != m_config.columnMapping.end()) {
                fieldName = it->second;
            } else {
                fieldName = "Column_" + std::to_string(j + 1);
            }
            
            pointData[fieldName] = row[j];
        }
        
        m_dataModel->addDataPoint(pointData);
        m_stats.validPoints++;
    }
    
    m_stats.totalPoints += newData.size();
    updateDataReady();
    
    return true;
}

bool CustomDataSource::updateDataPoint(size_t index, const std::vector<double>& newValues) {
    if (index >= m_dataModel->size() || newValues.empty()) {
        return false;
    }
    
    // 获取当前数据点
    std::map<std::string, double> currentPoint;
    if (!m_dataModel->getDataPoint(index, currentPoint)) {
        return false;
    }
    
    // 更新值
    for (size_t i = 0; i < newValues.size() && i < currentPoint.size(); ++i) {
        std::string fieldName;
        
        // 查找列映射
        std::map<int, std::string>::const_iterator it = m_config.columnMapping.find(i);
        if (it != m_config.columnMapping.end()) {
            fieldName = it->second;
        } else {
            fieldName = "Column_" + std::to_string(i + 1);
        }
        
        // 更新字段值
        std::map<std::string, double>::iterator fieldIt = currentPoint.find(fieldName);
        if (fieldIt != currentPoint.end()) {
            fieldIt->second = newValues[i];
        }
    }
    
    // 由于DataModel不支持直接更新单个点，需要重新构建数据
    // 这里简化实现：删除旧点，添加新点
    // 实际应用中可能需要扩展DataModel来支持更新操作
    
    return true;
}

CustomDataSource::DataStats CustomDataSource::getStatistics() const {
    DataStats stats = m_stats;
    
    // 计算数据范围
    std::vector<std::string> fieldNames = m_dataModel->getFieldNames();
    for (size_t i = 0; i < fieldNames.size(); ++i) {
        const std::string& fieldName = fieldNames[i];
        const DataModel::DataSeries& series = m_dataModel->getDataSeries(fieldName);
        
        if (series.empty()) {
            continue;
        }
        
        double minVal = series[0];
        double maxVal = series[0];
        
        for (size_t j = 0; j < series.size(); ++j) {
            minVal = std::min(minVal, series[j]);
            maxVal = std::max(maxVal, series[j]);
        }
        
        stats.ranges[fieldName] = std::make_pair(minVal, maxVal);
    }
    
    return stats;
}

void CustomDataSource::updateDataReady() {
    m_hasNewData = true;
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
}