#ifndef FILTERPLUGIN_H
#define FILTERPLUGIN_H

#include "PluginInterface.h"
#include <vector>
#include <map>

/**
 * @brief 滤波插件基类
 */
class FilterPlugin : public PluginInterface {
public:
    FilterPlugin();
    virtual ~FilterPlugin() = default;
    
    // PluginInterface 实现
    PluginType getType() const override { return PluginType::FILTER; }
    bool supportsRealTime() const override { return true; }
    bool supportsBatchProcessing() const override { return true; }
    
    // 滤波特定方法
    virtual void setCutoffFrequency(double freq) = 0;
    virtual double getCutoffFrequency() const = 0;
    virtual void setFilterOrder(int order) = 0;
    virtual int getFilterOrder() const = 0;
    
protected:
    double m_cutoffFrequency;
    int m_filterOrder;
    bool m_initialized;
};

/**
 * @brief 移动平均滤波插件
 */
class MovingAverageFilter : public FilterPlugin {
public:
    MovingAverageFilter();
    ~MovingAverageFilter() override;
    
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
    
    // FilterPlugin 实现
    void setCutoffFrequency(double freq) override;
    double getCutoffFrequency() const override;
    void setFilterOrder(int order) override;
    int getFilterOrder() const override;
    
    // 实时处理
    double processSample(double input);
    
private:
    std::vector<double> m_buffer;
    size_t m_windowSize;
    size_t m_currentIndex;
    double m_sum;
    mutable std::string m_lastError;
    int m_processingTime;
    size_t m_processedCount;
    
    void updateBuffer(double newValue);
    void initializeBuffer();
};

/**
 * @brief 低通滤波插件
 */
class LowPassFilter : public FilterPlugin {
public:
    LowPassFilter();
    ~LowPassFilter() override;
    
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
    
    // 滤波特定方法
    void setCutoffFrequency(double freq) override;
    double getCutoffFrequency() const override;
    void setFilterOrder(int order) override;
    int getFilterOrder() const override;
    
private:
    std::vector<double> m_previousInputs;
    std::vector<double> m_previousOutputs;
    std::vector<double> m_coefficientsA;
    std::vector<double> m_coefficientsB;
    mutable std::string m_lastError;
    int m_processingTime;
    size_t m_processedCount;
    
    void calculateCoefficients();
};

#endif // FILTERPLUGIN_H