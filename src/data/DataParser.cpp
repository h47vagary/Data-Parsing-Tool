#include "DataParser.h"
#include <sstream>
#include <algorithm>

bool DefaultDataParser::parseLine(const std::string& line, std::vector<double>& values) {
    std::istringstream ss(line);
    std::string token;
    std::vector<double> result;
    
    while (std::getline(ss, token, m_delimiter)) {
        // 去除空白字符
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        
        if (token.empty()) {
            continue;
        }
        
        try {
            double value = std::stod(token);
            result.push_back(value);
        } catch (const std::exception&) {
            return false;
        }
    }
    
    values = std::move(result);
    return true;
}

void DefaultDataParser::setConfig(const std::string& config) {
    // 简化实现：只支持设置分隔符
    if (!config.empty()) {
        m_delimiter = config[0];
    }
}

bool DefaultDataParser::validateFormat(const std::string& line) {
    // 检查是否包含数字字符
    return std::any_of(line.begin(), line.end(), [](char c) {
        return std::isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
    });
}