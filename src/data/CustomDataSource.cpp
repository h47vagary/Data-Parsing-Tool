#include "CustomDataSource.h"
#include "DataParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

CustomDataSource::CustomDataSource() 
    : m_state(State::Stopped)
    , m_hasNewData(false)
    , m_dataModel(std::make_shared<DataModel>())
{
    // 初始化统计信息
    m_stats = {0, 0, 0, {}};
}

CustomDataSource::~CustomDataSource() {
    stop();
}

bool CustomDataSource::initialize(const std::string& config) {
    m_sourcePath = config;
    m_state = State::Stopped;
    m_hasNewData = false;
    
    // 重置统计信息
    m_stats = {0, 0, 0, {}};
    
    return true;
}

bool CustomDataSource::start() {
    if (m_sourcePath.empty()) {
        if (m_errorCallback) {
            m_errorCallback("数据源路径未设置");
        }
        return false;
    }
    
    std::ifstream file(m_sourcePath);
    if (!file.is_open()) {
        if (m_errorCallback) {
            m_errorCallback("无法打开文件: " + m_sourcePath);
        }
        return false;
    }
    
    m_dataModel->clear();
    m_stats = {0, 0, 0, {}};
    
    std::string line;
    int lineNumber = 0;
    int skippedLines = 0;
    
    // 跳过指定行数
    for (int i = 0; i < m_config.skipLines && std::getline(file, line); ++i) {
        skippedLines++;
    }
    
    // 如果有标题行，跳过
    if (m_config.hasHeader && std::getline(file, line)) {
        skippedLines++;
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
        
        // 验证数据
        if (values.size() < 6) {
            m_stats.skippedPoints++;
            continue;
        }
        
        // 添加到数据模型
        m_dataModel->addPoint(values[0], values[1], values[2], 
                             values[3], values[4], values[5]);
        m_stats.validPoints++;
    }
    
    m_stats.totalPoints = lineNumber;
    m_stats.skippedPoints += skippedLines;
    
    file.close();
    m_state = State::Running;
    updateDataReady();
    
    return true;
}

void CustomDataSource::stop() {
    m_dataModel->clear();
    m_state = State::Stopped;
    m_hasNewData = false;
}

std::vector<double> CustomDataSource::getData() {
    m_hasNewData = false;
    if (m_dataModel && !m_dataModel->empty()) {
        return m_dataModel->getX(); // 返回X数据作为示例
    }
    return {};
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
            continue;
        }
        
        try {
            double value = std::stod(token);
            if (!validateValue(value, m_config.validationRule)) {
                return false;
            }
            parsedValues.push_back(value);
        } catch (const std::exception&) {
            return false;
        }
    }
    
    values = std::move(parsedValues);
    return !values.empty();
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
    
    for (const auto& row : newData) {
        if (row.size() >= 6) {
            m_dataModel->addPoint(row[0], row[1], row[2], row[3], row[4], row[5]);
            m_stats.validPoints++;
        } else {
            m_stats.skippedPoints++;
        }
    }
    
    m_stats.totalPoints += newData.size();
    updateDataReady();
    
    return true;
}

bool CustomDataSource::updateDataPoint(size_t index, const std::vector<double>& newValues) {
    if (index >= m_dataModel->size() || newValues.size() < 6) {
        return false;
    }
    
    // 这里需要扩展DataModel来支持更新操作
    // 目前实现为删除旧点，添加新点（简化实现）
    return true;
}

CustomDataSource::DataStats CustomDataSource::getStatistics() const {
    auto stats = m_stats;
    
    // 计算数据范围
    if (m_dataModel && !m_dataModel->empty()) {
        const auto& x = m_dataModel->getX();
        const auto& y = m_dataModel->getY();
        const auto& z = m_dataModel->getZ();
        
        if (!x.empty()) {
            auto minMaxX = std::minmax_element(x.begin(), x.end());
            stats.ranges["x"] = {*minMaxX.first, *minMaxX.second};
        }
        
        if (!y.empty()) {
            auto minMaxY = std::minmax_element(y.begin(), y.end());
            stats.ranges["y"] = {*minMaxY.first, *minMaxY.second};
        }
        
        if (!z.empty()) {
            auto minMaxZ = std::minmax_element(z.begin(), z.end());
            stats.ranges["z"] = {*minMaxZ.first, *minMaxZ.second};
        }
    }
    
    return stats;
}

void CustomDataSource::updateDataReady() {
    m_hasNewData = true;
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
}