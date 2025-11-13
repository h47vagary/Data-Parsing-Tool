#ifndef CSVDATASOURCE_H
#define CSVDATASOURCE_H

#include "DataSource.h"
#include "DataModel.h"
#include <memory>
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
    bool hasNewData() const override { return m_dataModel && !m_dataModel->empty(); }

private:
    bool parseDouble(const std::string& str, double& value);

    std::string m_filename;
    char m_delimiter;
    std::shared_ptr<DataModel> m_dataModel;
    State m_state;
};

#endif // CSVDATASOURCE_H