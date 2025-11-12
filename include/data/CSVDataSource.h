#ifndef CSVDATASOURCE_H
#define CSVDATASOURCE_H

#include "DataSource.h"
#include <vector>
#include <string>

class CSVDataSource : public DataSource {
public:
    CSVDataSource();
    virtual ~CSVDataSource() = default;
    
    bool initialize(const std::string& config) override;
    bool start() override;
    void stop() override;
    State getState() const override { return m_state; }
    
    std::vector<double> getData() override;
    bool hasNewData() const override { return !m_data.empty(); }

private:
    std::string m_filename;
    char m_delimiter;
    bool m_hasHeader;
    std::vector<double> m_data;
    State m_state;
};

#endif // CSVDATASOURCE_H