#pragma once

#include <vector>
#include <string>
#include <memory>

/**
 * @brief 数据解析器接口
 * 
 * 用于支持不同的数据格式解析策略
 */
class DataParser {
public:
    virtual ~DataParser() = default;

    /**
     * @brief 解析单行数据
     * 
     * @param line 输入的数据行
     * @param values 输出的数值数组
     * @return true 
     * @return false 
     */
    virtual bool parseLine(const std::string &line, std::vector<double> &values) = 0;

    /**
     * @brief 设置解析数据
     * 
     * @param config 配置参数（Json格式或其他）
     */
    virtual void setConfig(const std::string &config) = 0;

    /**
     * @brief 验证数据格式
     * 
     * @param line 待验证的数据行
     * @return true 满足格式要求
     * @return false 不满足格式要求
     */
    virtual bool validateFormat(const std::string& line) = 0;
};


class DefaultDataParser : public DataParser {
public:
    bool parseLine(const std::string &line, std::vector<double> &values) override;
    void setConfig(const std::string &config) override;
    bool validateFormat(const std::string &line) override;

private:
    char m_delimiter = ',';
};