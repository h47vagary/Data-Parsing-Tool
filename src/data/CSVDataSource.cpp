#include "CSVDataSource.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>

CSVDataSource::CSVDataSource() 
    : m_delimiter(','), m_state(State::Stopped), m_dataModel(std::make_shared<DataModel>()) {}

bool CSVDataSource::initialize(const std::string& config) {
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

    m_dataModel->clear();
    
    std::string line;
    // 跳过标题行
    if (!std::getline(file, line)) {
        if (m_errorCallback) {
            m_errorCallback("文件为空或读取失败: " + m_filename);
        }
        return false;
    }

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        double in_x, in_y, in_z, in_a, in_b, in_c;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_x)) continue;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_y)) continue;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_z)) continue;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_a)) continue;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_b)) continue;

        if (!std::getline(ss, token, m_delimiter)) continue;
        if (!parseDouble(token, in_c)) continue;

        m_dataModel->addPoint(in_x, in_y, in_z, in_a, in_b, in_c);
    }

    file.close();
    m_state = State::Running;
    
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
    
    return true;
}

void CSVDataSource::stop() {
    m_dataModel->clear();
    m_state = State::Stopped;
}

std::vector<double> CSVDataSource::getData() {
    return m_dataModel->getX(); // 返回X坐标数据作为示例
}

bool CSVDataSource::parseDouble(const std::string& str, double& value) {
    try {
        size_t pos;
        value = std::stod(str, &pos);
        return pos == str.length();
    } catch (const std::exception&) {
        return false;
    }
}