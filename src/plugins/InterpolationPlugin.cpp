#include "InterpolationPlugin.h"
#include "DataModel.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// ==================== LinearInterpolationPlugin ====================

LinearInterpolationPlugin::LinearInterpolationPlugin() 
    : m_method("linear"), m_stepSize(1.0), m_processingTime(0), m_processedCount(0) {
}

LinearInterpolationPlugin::~LinearInterpolationPlugin() {
    shutdown();
}

std::string LinearInterpolationPlugin::getName() const {
    return "LinearInterpolationPlugin";
}

std::string LinearInterpolationPlugin::getVersion() const {
    return "1.0.0";
}

std::string LinearInterpolationPlugin::getDescription() const {
    return "线性插值插件，用于数据点插值和重采样";
}

std::string LinearInterpolationPlugin::getAuthor() const {
    return "Data Parsing Tool Team";
}

std::vector<std::string> LinearInterpolationPlugin::getDependencies() const {
    return {};
}

bool LinearInterpolationPlugin::initialize() {
    m_lastError.clear();
    return true;
}

bool LinearInterpolationPlugin::shutdown() {
    m_processedCount = 0;
    m_processingTime = 0;
    return true;
}

bool LinearInterpolationPlugin::isInitialized() const {
    return true;
}

bool LinearInterpolationPlugin::processData(std::shared_ptr<DataModel> input, 
                                           std::shared_ptr<DataModel> output) {
    if (!input || !output) {
        m_lastError = "输入输出数据为空";
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        auto fieldNames = input->getFieldNames();
        if (fieldNames.empty()) {
            m_lastError = "输入数据没有字段";
            return false;
        }
        
        // 检查是否有时间字段
        bool hasTimeField = false;
        for (const auto& fieldName : fieldNames) {
            if (fieldName == "time" || fieldName == "Time" || fieldName == "TIME") {
                hasTimeField = true;
                break;
            }
        }
        
        if (hasTimeField) {
            // 有时时间字段的插值
            return processWithTimeField(input, output, fieldNames);
        } else {
            // 无时间字段的插值（基于索引）
            return processWithoutTimeField(input, output, fieldNames);
        }
        
    } catch (const std::exception& e) {
        m_lastError = std::string("插值处理失败: ") + e.what();
        return false;
    }
}

bool LinearInterpolationPlugin::processWithTimeField(std::shared_ptr<DataModel> input, 
                                                   std::shared_ptr<DataModel> output,
                                                   const std::vector<std::string>& fieldNames) {
    // 获取时间数据
    const auto& timeData = input->getDataSeries("time");
    if (timeData.empty()) {
        m_lastError = "时间字段数据为空";
        return false;
    }
    
    // 生成新的时间序列
    double startTime = timeData.front();
    double endTime = timeData.back();
    std::vector<double> newTime;
    
    for (double t = startTime; t <= endTime; t += m_stepSize) {
        newTime.push_back(t);
    }
    
    // 为每个字段进行插值
    for (const auto& fieldName : fieldNames) {
        if (fieldName == "time") {
            output->addDataSeries("time", newTime);
            continue;
        }
        
        const auto& yData = input->getDataSeries(fieldName);
        if (yData.size() != timeData.size()) {
            m_lastError = "时间序列和数据序列长度不匹配";
            return false;
        }
        
        std::vector<double> newYData;
        newYData.reserve(newTime.size());
        
        if (!linearInterpolate(timeData, yData, newTime, newYData)) {
            return false;
        }
        
        output->addDataSeries(fieldName, newYData);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    m_processedCount += newTime.size();
    
    m_lastError.clear();
    return true;
}

bool LinearInterpolationPlugin::processWithoutTimeField(std::shared_ptr<DataModel> input, 
                                                      std::shared_ptr<DataModel> output,
                                                      const std::vector<std::string>& fieldNames) {
    // 基于索引的插值
    size_t originalSize = input->size();
    if (originalSize == 0) {
        m_lastError = "输入数据为空";
        return false;
    }
    
    // 生成新的索引序列
    size_t newSize = static_cast<size_t>(originalSize * (1.0 / m_stepSize));
    std::vector<double> newX(newSize);
    for (size_t i = 0; i < newSize; ++i) {
        newX[i] = static_cast<double>(i) * m_stepSize;
    }
    
    // 为每个字段进行插值
    for (const auto& fieldName : fieldNames) {
        const auto& yData = input->getDataSeries(fieldName);
        if (yData.size() != originalSize) {
            m_lastError = "数据字段长度不一致";
            return false;
        }
        
        // 生成原始索引
        std::vector<double> originalX(originalSize);
        for (size_t i = 0; i < originalSize; ++i) {
            originalX[i] = static_cast<double>(i);
        }
        
        std::vector<double> newYData;
        if (!linearInterpolate(originalX, yData, newX, newYData)) {
            return false;
        }
        
        output->addDataSeries(fieldName, newYData);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    m_processedCount += newSize;
    
    m_lastError.clear();
    return true;
}

bool LinearInterpolationPlugin::linearInterpolate(const std::vector<double>& x, 
                                                 const std::vector<double>& y,
                                                 std::vector<double>& newX, 
                                                 std::vector<double>& newY) {
    if (x.size() != y.size() || x.size() < 2) {
        m_lastError = "输入数据无效";
        return false;
    }
    
    newY.clear();
    newY.reserve(newX.size());
    
    size_t j = 0; // 原始数据索引
    
    for (size_t i = 0; i < newX.size(); ++i) {
        double targetX = newX[i];
        
        // 找到目标点所在的区间
        while (j < x.size() - 1 && x[j + 1] < targetX) {
            j++;
        }
        
        if (j >= x.size() - 1) {
            // 超出范围，使用最后一个值
            newY.push_back(y.back());
            continue;
        }
        
        if (targetX < x.front()) {
            // 小于最小值，使用第一个值
            newY.push_back(y.front());
            continue;
        }
        
        // 线性插值: y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
        double x0 = x[j];
        double x1 = x[j + 1];
        double y0 = y[j];
        double y1 = y[j + 1];
        
        if (x1 == x0) {
            // 避免除零
            newY.push_back(y0);
        } else {
            double interpolatedY = y0 + (y1 - y0) * (targetX - x0) / (x1 - x0);
            newY.push_back(interpolatedY);
        }
    }
    
    return true;
}

bool LinearInterpolationPlugin::setParameter(const std::string& key, const QVariant& value) {
    if (key == "method") {
        QString method = value.toString();
        if (method == "linear") {
            m_method = "linear";
            return true;
        }
    } else if (key == "step_size") {
        bool ok;
        double step = value.toDouble(&ok);
        if (ok && step > 0) {
            m_stepSize = step;
            return true;
        }
    }
    
    m_lastError = "无效参数: " + key;
    return false;
}

QVariant LinearInterpolationPlugin::getParameter(const std::string& key) const {
    if (key == "method") {
        return QString::fromStdString(m_method);
    } else if (key == "step_size") {
        return m_stepSize;
    }
    return QVariant();
}

std::map<std::string, QVariant> LinearInterpolationPlugin::getDefaultParameters() const {
    return {
        {"method", "linear"},
        {"step_size", 1.0}
    };
}

bool LinearInterpolationPlugin::validateParameters() const {
    return m_stepSize > 0 && (m_method == "linear");
}

std::string LinearInterpolationPlugin::getLastError() const {
    return m_lastError;
}

int LinearInterpolationPlugin::getProcessingTime() const {
    return m_processingTime;
}

size_t LinearInterpolationPlugin::getProcessedCount() const {
    return m_processedCount;
}

void LinearInterpolationPlugin::setInterpolationMethod(const std::string& method) {
    if (method == "linear") {
        m_method = method;
    } else {
        m_lastError = "不支持的插值方法: " + method;
    }
}

std::string LinearInterpolationPlugin::getInterpolationMethod() const {
    return m_method;
}