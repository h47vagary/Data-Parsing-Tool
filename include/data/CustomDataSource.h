#ifndef CUSTOMDATASOURCE_H
#define CUSTOMDATASOURCE_H

#include "DataSource.h"
#include "DataModel.h"
#include "DataParser.h"
#include <string>
#include <memory>
#include <vector>
#include <map>

/**
 * @brief 自定义数据源类，支持多种自定义数据格式
 * 
 * 支持功能：
 * - 自定义分隔符
 * - 自定义数据列映射
 * - 支持跳过行和注释行
 * - 支持数据验证和过滤
 */
class CustomDataSource : public DataSource {
public:
    /**
     * @brief 数据解析配置结构
     */
    struct ParseConfig {
        char delimiter = ',';           // 分隔符
        char commentChar = '#';         // 注释字符
        bool hasHeader = true;          // 是否有标题行
        int skipLines = 0;              // 跳过的行数
        std::string encoding = "UTF-8"; // 文件编码
        
        // 列映射：列索引 -> 数据字段
        std::map<int, std::string> columnMapping = {
            {0, "x"}, {1, "y"}, {2, "z"}, 
            {3, "a"}, {4, "b"}, {5, "c"}
        };
        
        // 数据验证规则
        struct ValidationRule {
            double minValue = -1e9;     // 最小值
            double maxValue = 1e9;     // 最大值
            bool allowNaN = false;      // 是否允许NaN
            bool allowInfinity = false; // 是否允许无穷大
        } validationRule;
    };

    CustomDataSource();
    ~CustomDataSource();
    
    // DataSource 接口实现
    bool initialize(const std::string& config) override;
    bool start() override;
    void stop() override;
    State getState() const override { return m_state; }
    
    std::vector<double> getData() override;
    bool hasNewData() const override { return m_hasNewData; }
    
    // 自定义配置方法
    void setParseConfig(const ParseConfig& config) { m_config = config; }
    const ParseConfig& getParseConfig() const { return m_config; }
    
    void setCustomParser(std::unique_ptr<DataParser> parser);
    
    // 数据访问
    std::shared_ptr<DataModel> getDataModel() const { return m_dataModel; }
    
    // 批量数据操作
    bool appendData(const std::vector<std::vector<double>>& newData);
    bool updateDataPoint(size_t index, const std::vector<double>& newValues);
    
    // 数据统计
    struct DataStats {
        size_t totalPoints;
        size_t validPoints;
        size_t skippedPoints;
        std::map<std::string, std::pair<double, double>> ranges; // 各字段范围
    };
    
    DataStats getStatistics() const;

private:
    bool parseLine(const std::string& line, std::vector<double>& values);
    bool validateValue(double value, const ParseConfig::ValidationRule& rule);
    void updateDataReady();
    
    std::string m_sourcePath;
    ParseConfig m_config;
    std::shared_ptr<DataModel> m_dataModel;
    State m_state;
    bool m_hasNewData;
    
    std::unique_ptr<DataParser> m_customParser;
    DataStats m_stats;
};

#endif // CUSTOMDATASOURCE_H