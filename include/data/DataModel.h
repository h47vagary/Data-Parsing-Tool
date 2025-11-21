#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

class DataModel {
public:
    // 支持的数据类型
    typedef std::vector<double> DataSeries;
    
    // 元数据类型
    struct MetadataValue {
        enum Type { STRING, INT, DOUBLE, BOOL };
        Type type;
        std::string stringValue;
        int intValue;
        double doubleValue;
        bool boolValue;
        
        MetadataValue() : type(STRING), intValue(0), doubleValue(0.0), boolValue(false) {}
        MetadataValue(const std::string& val) : type(STRING), stringValue(val), intValue(0), doubleValue(0.0), boolValue(false) {}
        MetadataValue(int val) : type(INT), intValue(val), doubleValue(0.0), boolValue(false) {}
        MetadataValue(double val) : type(DOUBLE), doubleValue(val), intValue(0), boolValue(false) {}
        MetadataValue(bool val) : type(BOOL), boolValue(val), intValue(0), doubleValue(0.0) {}
    };
    
    DataModel();
    
    // === 字段管理 ===
    void addField(const std::string& fieldName);
    void removeField(const std::string& fieldName);
    bool hasField(const std::string& fieldName) const;
    std::vector<std::string> getFieldNames() const;
    
    // === 数据操作 ===
    void clear();
    void clearField(const std::string& fieldName);
    
    // 添加单点数据
    void addDataPoint(const std::map<std::string, double>& pointData);
    
    // 批量添加数据
    void addDataSeries(const std::string& fieldName, const DataSeries& data);
    void addDataPoints(const std::vector<std::map<std::string, double> >& points);
    
    // === 数据访问 ===
    const DataSeries& getDataSeries(const std::string& fieldName) const;
    double getValue(const std::string& fieldName, size_t index) const;
    bool getDataPoint(size_t index, std::map<std::string, double>& point) const;
    
    // === 元数据管理 ===
    void setFieldMetadata(const std::string& fieldName, const std::string& key, const MetadataValue& value);
    MetadataValue getFieldMetadata(const std::string& fieldName, const std::string& key) const;
    
    void setFieldColor(const std::string& fieldName, const std::string& color);
    void setFieldVisible(const std::string& fieldName, bool visible);
    
    // === 数据验证和统计 ===
    bool isValid() const;
    size_t size() const;
    bool empty() const;
    
    struct Statistics {
        size_t totalPoints;
        size_t validPoints;
        std::map<std::string, std::pair<double, double> > ranges; // min/max
        std::map<std::string, double> averages;
        
        Statistics() : totalPoints(0), validPoints(0) {}
    };
    
    Statistics calculateStatistics() const;
    
    // === 数据子集 ===
    std::shared_ptr<DataModel> getSubset(size_t startIndex, size_t endIndex) const;
    std::shared_ptr<DataModel> getSubsetByFields(const std::vector<std::string>& fieldNames) const;

private:
    std::map<std::string, DataSeries> m_dataSeries;
    std::map<std::string, std::map<std::string, MetadataValue> > m_fieldMetadata;
    size_t m_pointCount;
    
    bool checkConsistency() const;
    static DataSeries s_emptySeries; // 静态空数据，用于返回引用
};

#endif // DATAMODEL_H