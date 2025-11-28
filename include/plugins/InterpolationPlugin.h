#ifndef INTERPOLATIONPLUGIN_H
#define INTERPOLATIONPLUGIN_H

#include "PluginInterface.h"

/**
 * @brief 插值插件基类
 */
class InterpolationPlugin : public PluginInterface {
public:
    InterpolationPlugin();
    virtual ~InterpolationPlugin() = default;
    
    PluginType getType() const override { return PluginType::INTERPOLATION; }
    bool supportsRealTime() const override { return false; }
    bool supportsBatchProcessing() const override { return true; }
    
    virtual void setInterpolationMethod(const std::string& method) = 0;
    virtual std::string getInterpolationMethod() const = 0;
};

/**
 * @brief 线性插值插件
 */
class LinearInterpolationPlugin : public InterpolationPlugin {
public:
    LinearInterpolationPlugin();
    ~LinearInterpolationPlugin() override;
    
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
    
    // InterpolationPlugin 实现
    void setInterpolationMethod(const std::string& method) override;
    std::string getInterpolationMethod() const override;
    
private:
    std::string m_method;
    double m_stepSize;
    mutable std::string m_lastError;
    int m_processingTime;
    size_t m_processedCount;
    
    bool linearInterpolate(const std::vector<double>& x, const std::vector<double>& y,
                          std::vector<double>& newX, std::vector<double>& newY);
};

#endif // INTERPOLATIONPLUGIN_H