#ifndef CSVDATASOURCE_H
#define CSVDATASOURCE_H

#include "DataSource.h"
#include "DataModel.h"
#include <string>
#include <memory>

class CSVDataSource : public DataSource {
public:
    CSVDataSource();
    
    // DataSource 接口实现
    bool initialize(const std::string& config) override;
    bool start() override;
    void stop() override;
    State getState() const override { return m_state; }
    
    std::vector<double> getData() override;
    bool hasNewData() const override { return m_dataModel && !m_dataModel->empty(); }
    
    // CSV 特定配置
    void setDelimiter(char delimiter) { m_delimiter = delimiter; }
    void setHasHeader(bool hasHeader) { m_hasHeader = hasHeader; }
    void setSkipLines(int skipLines) { m_skipLines = skipLines; }
    
    // 数据访问
    std::shared_ptr<DataModel> getDataModel() const { return m_dataModel; }
    const std::vector<std::string>& getHeaders() const { return m_headers; }
    
    // 数据验证和统计
    struct ParseResult {
        bool success;
        int totalLines;
        int validLines;
        int skippedLines;
        std::string errorMessage;
        
        ParseResult() : success(false), totalLines(0), validLines(0), skippedLines(0) {}
    };
    
    ParseResult getParseResult() const { return m_parseResult; }

private:
    bool parseLine(const std::string& line, std::vector<double>& values);
    bool parseDouble(const std::string& str, double& value);
    void detectDelimiter(const std::string& firstLine);
    void extractHeaders(const std::string& headerLine);
    
    std::string m_filename;
    char m_delimiter;
    bool m_hasHeader;
    int m_skipLines;
    std::shared_ptr<DataModel> m_dataModel;
    State m_state;
    
    std::vector<std::string> m_headers;
    ParseResult m_parseResult;
};

#endif // CSVDATASOURCE_H