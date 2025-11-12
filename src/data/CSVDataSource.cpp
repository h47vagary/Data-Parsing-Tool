#include "CSVDataSource.h"
#include <fstream>
#include <sstream>
#include <iostream>

CSVDataSource::CSVDataSource() : m_delimiter(','), m_hasHeader(false) {}

bool CSVDataSource::initialize(const std::string& config) {
    // 解析配置，这里简化处理
    m_filename = config;
    return true;
}

bool CSVDataSource::start() {
    std::ifstream file(m_filename);
    if (!file.is_open()) {
        if (m_errorCallback) {
            m_errorCallback("无法打开文件: " + m_filename);
        }
        return false;
    }
    
    std::string line;
    m_data.clear();
    
    // 读取数据
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        std::vector<double> row;
        
        while (std::getline(ss, value, m_delimiter)) {
            try {
                row.push_back(std::stod(value));
            } catch (const std::exception& e) {
                // 忽略转换错误
            }
        }
        
        if (!row.empty()) {
            m_data.insert(m_data.end(), row.begin(), row.end());
        }
    }
    
    file.close();
    
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
    
    return true;
}

void CSVDataSource::stop() {
    m_data.clear();
}

std::vector<double> CSVDataSource::getData() {
    return m_data;
}