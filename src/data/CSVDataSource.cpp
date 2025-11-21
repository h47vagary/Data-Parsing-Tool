#include "CSVDataSource.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

CSVDataSource::CSVDataSource() 
    : m_delimiter(','), m_hasHeader(true), m_skipLines(0), 
      m_state(State::Stopped), m_dataModel(std::make_shared<DataModel>()) {}

bool CSVDataSource::initialize(const std::string& config) {
    m_filename = config;
    m_state = State::Stopped;
    m_headers.clear();
    m_parseResult = ParseResult();
    
    return true;
}

bool CSVDataSource::start() {
    if (m_filename.empty()) {
        if (m_errorCallback) {
            m_errorCallback("文件名不能为空");
        }
        return false;
    }
    
    std::ifstream file(m_filename.c_str());
    if (!file.is_open()) {
        if (m_errorCallback) {
            m_errorCallback("无法打开文件: " + m_filename);
        }
        return false;
    }
    
    // 重置数据模型
    m_dataModel->clear();
    m_headers.clear();
    m_parseResult = ParseResult();
    
    std::string line;
    int lineNumber = 0;
    int skippedLines = 0;
    int validLines = 0;
    
    // 跳过指定行数
    for (int i = 0; i < m_skipLines && std::getline(file, line); ++i) {
        skippedLines++;
        lineNumber++;
    }
    
    // 读取第一行用于检测分隔符和表头
    if (std::getline(file, line)) {
        lineNumber++;
        
        // 自动检测分隔符（如果未设置）
        if (m_delimiter == '\0') {
            detectDelimiter(line);
        }
        
        if (m_hasHeader) {
            extractHeaders(line);
            skippedLines++;
        } else {
            // 如果没有表头，回退到文件开头重新解析
            file.clear();
            file.seekg(0, std::ios::beg);
            
            // 重新跳过指定行数
            for (int i = 0; i < m_skipLines && std::getline(file, line); ++i) {
                // 已经跳过，这里只是移动文件指针
            }
        }
    }
    
    // 解析数据行
    while (std::getline(file, line)) {
        lineNumber++;
        
        // 跳过空行和注释行（以#开头）
        if (line.empty() || line[0] == '#') {
            skippedLines++;
            continue;
        }
        
        std::vector<double> values;
        if (parseLine(line, values)) {
            if (values.empty()) {
                skippedLines++;
                continue;
            }
            
            // 创建数据点
            std::map<std::string, double> point;
            
            // 如果有表头，使用表头作为字段名
            if (!m_headers.empty()) {
                for (size_t i = 0; i < values.size() && i < m_headers.size(); ++i) {
                    point[m_headers[i]] = values[i];
                }
                
                // 如果值比表头多，为多余的列生成字段名
                for (size_t i = m_headers.size(); i < values.size(); ++i) {
                    std::string fieldName = "Column_" + std::to_string(i + 1);
                    point[fieldName] = values[i];
                }
            } else {
                // 没有表头，使用默认字段名
                for (size_t i = 0; i < values.size(); ++i) {
                    std::string fieldName = "Column_" + std::to_string(i + 1);
                    point[fieldName] = values[i];
                }
            }
            
            m_dataModel->addDataPoint(point);
            validLines++;
        } else {
            skippedLines++;
        }
    }
    
    file.close();
    
    // 更新解析结果
    m_parseResult.success = true;
    m_parseResult.totalLines = lineNumber;
    m_parseResult.validLines = validLines;
    m_parseResult.skippedLines = skippedLines;
    
    m_state = State::Running;
    
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
    
    std::cout << "CSV文件解析完成: " << validLines << " 行有效数据" << std::endl;
    
    return true;
}

void CSVDataSource::stop() {
    m_dataModel->clear();
    m_state = State::Stopped;
}

std::vector<double> CSVDataSource::getData() {
    // 返回第一个字段的数据作为示例
    auto fieldNames = m_dataModel->getFieldNames();
    if (!fieldNames.empty()) {
        return m_dataModel->getDataSeries(fieldNames[0]);
    }
    return std::vector<double>();
}

bool CSVDataSource::parseLine(const std::string& line, std::vector<double>& values) {
    std::istringstream ss(line);
    std::string token;
    std::vector<double> parsedValues;
    
    while (std::getline(ss, token, m_delimiter)) {
        // 去除首尾空白字符
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        
        if (token.empty()) {
            // 空字段，跳过或使用默认值
            continue;
        }
        
        double value;
        if (parseDouble(token, value)) {
            parsedValues.push_back(value);
        } else {
            // 解析失败，跳过这一行
            return false;
        }
    }
    
    if (!parsedValues.empty()) {
        values = parsedValues;
        return true;
    }
    
    return false;
}

bool CSVDataSource::parseDouble(const std::string& str, double& value) {
    if (str.empty()) {
        return false;
    }
    
    try {
        size_t pos;
        value = std::stod(str, &pos);
        
        // 检查是否整个字符串都被成功转换
        if (pos != str.length()) {
            // 可能有尾随字符，尝试再次检查
            std::string remaining = str.substr(pos);
            // 去除尾随空白字符后再次检查
            remaining.erase(0, remaining.find_first_not_of(" \t\r\n"));
            if (!remaining.empty()) {
                return false; // 有非空白尾随字符，转换失败
            }
        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void CSVDataSource::detectDelimiter(const std::string& firstLine) {
    // 常见分隔符
    const char delimiters[] = {',', ';', '\t', '|', ' '};
    int maxCount = 0;
    char detectedDelimiter = ',';
    
    for (size_t i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]); ++i) {
        char delim = delimiters[i];
        int count = 0;
        
        for (size_t j = 0; j < firstLine.length(); ++j) {
            if (firstLine[j] == delim) {
                count++;
            }
        }
        
        if (count > maxCount) {
            maxCount = count;
            detectedDelimiter = delim;
        }
    }
    
    m_delimiter = detectedDelimiter;
    std::cout << "检测到分隔符: '" << m_delimiter << "'" << std::endl;
}

void CSVDataSource::extractHeaders(const std::string& headerLine) {
    std::istringstream ss(headerLine);
    std::string token;
    m_headers.clear();
    
    while (std::getline(ss, token, m_delimiter)) {
        // 清理表头
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        
        // 如果表头为空，生成默认名称
        if (token.empty()) {
            token = "Column_" + std::to_string(m_headers.size() + 1);
        }
        
        m_headers.push_back(token);
    }
    
    std::cout << "检测到 " << m_headers.size() << " 个字段: ";
    for (size_t i = 0; i < m_headers.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << m_headers[i];
    }
    std::cout << std::endl;
}