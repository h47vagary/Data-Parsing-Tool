#ifndef EXPORTPLUGIN_H
#define EXPORTPLUGIN_H

#include "PluginInterface.h"

/**
 * @brief 导出插件基类
 */
class ExportPlugin : public PluginInterface {
public:
    ExportPlugin();
    virtual ~ExportPlugin() = default;
    
    PluginType getType() const override { return PluginType::EXPORT; }
    bool supportsRealTime() const override { return false; }
    bool supportsBatchProcessing() const override { return true; }
    
    virtual bool exportToFile(const std::string& filename, 
                            std::shared_ptr<DataModel> data) = 0;
    virtual std::vector<std::string> getSupportedFormats() const = 0;
};

/**
 * @brief CSV导出插件
 */
class CSVExportPlugin : public ExportPlugin {
public:
    CSVExportPlugin();
    ~CSVExportPlugin() override;
    
    // PluginInterface 实现
    std::string getName() const override;
    std::string getVersion() const override;
    std::string getDescription() const override;
    std::string getAuthor() const override;
    std::vector<std::string> getDependencies() const override;
    
    bool initialize() override;
    bool shutdown() override;
    bool isInitialized() const override;
    
    bool processData(std::shared_ptr<DataModel> input, 
                    std::shared_ptr<DataModel> output) override;
    
    bool setParameter(const std::string& key, const QVariant& value) override;
    QVariant getParameter(const std::string& key) const override;
    std::map<std::string, QVariant> getDefaultParameters() const override;
    bool validateParameters() const override;
    
    std::string getLastError() const override;
    int getProcessingTime() const override;
    size_t getProcessedCount() const override;
    
    // ExportPlugin 实现
    bool exportToFile(const std::string& filename, 
                     std::shared_ptr<DataModel> data) override;
    std::vector<std::string> getSupportedFormats() const override;
    
private:
    std::string m_delimiter;
    bool m_includeHeader;
    std::string m_encoding;
    mutable std::string m_lastError;
    int m_processingTime;
    size_t m_processedCount;
    
    bool writeCSVFile(const std::string& filename, std::shared_ptr<DataModel> data);
};

#endif // EXPORTPLUGIN_H