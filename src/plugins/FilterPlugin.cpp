#include "FilterPlugin.h"
#include "DataModel.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

// ==================== MovingAverageFilter ====================

MovingAverageFilter::MovingAverageFilter() 
    : m_windowSize(5), m_currentIndex(0), m_sum(0.0), 
      m_initialized(false), m_processingTime(0), m_processedCount(0) {
    m_cutoffFrequency = 0.5;
    m_filterOrder = 1;
}

MovingAverageFilter::~MovingAverageFilter() {
    shutdown();
}

std::string MovingAverageFilter::getName() const {
    return "MovingAverageFilter";
}

std::string MovingAverageFilter::getVersion() const {
    return "1.0.0";
}

std::string MovingAverageFilter::getDescription() const {
    return "移动平均滤波插件，用于平滑数据噪声";
}

std::string MovingAverageFilter::getAuthor() const {
    return "Data Parsing Tool Team";
}

std::vector<std::string> MovingAverageFilter::getDependencies() const {
    return {}; // 无依赖
}

bool MovingAverageFilter::initialize() {
    if (m_initialized) {
        return true;
    }
    
    try {
        initializeBuffer();
        m_initialized = true;
        m_lastError.clear();
        return true;
    } catch (const std::exception& e) {
        m_lastError = std::string("初始化失败: ") + e.what();
        return false;
    }
}

bool MovingAverageFilter::shutdown() {
    m_buffer.clear();
    m_initialized = false;
    m_processedCount = 0;
    m_processingTime = 0;
    return true;
}

bool MovingAverageFilter::isInitialized() const {
    return m_initialized;
}

bool MovingAverageFilter::processData(std::shared_ptr<DataModel> input, 
                                     std::shared_ptr<DataModel> output) {
    if (!m_initialized || !input || !output) {
        m_lastError = "插件未初始化或输入输出为空";
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // 获取输入数据
        auto fieldNames = input->getFieldNames();
        if (fieldNames.empty()) {
            m_lastError = "输入数据没有字段";
            return false;
        }
        
        // 处理每个字段
        for (const auto& fieldName : fieldNames) {
            const auto& inputData = input->getDataSeries(fieldName);
            if (inputData.empty()) {
                continue;
            }
            
            std::vector<double> outputData;
            outputData.reserve(inputData.size());
            
            // 对每个数据点应用移动平均
            for (size_t i = 0; i < inputData.size(); ++i) {
                double filteredValue = processSample(inputData[i]);
                outputData.push_back(filteredValue);
            }
            
            // 将结果保存到输出
            output->addDataSeries(fieldName, outputData);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        m_processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        m_processedCount += input->size();
        
        m_lastError.clear();
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = std::string("处理数据失败: ") + e.what();
        return false;
    }
}

bool MovingAverageFilter::setParameter(const std::string& key, const QVariant& value) {
    if (key == "window_size") {
        bool ok;
        int newSize = value.toInt(&ok);
        if (ok && newSize > 0) {
            m_windowSize = static_cast<size_t>(newSize);
            if (m_initialized) {
                initializeBuffer();
            }
            return true;
        }
    } else if (key == "cutoff_frequency") {
        bool ok;
        double freq = value.toDouble(&ok);
        if (ok && freq > 0) {
            m_cutoffFrequency = freq;
            return true;
        }
    } else if (key == "filter_order") {
        bool ok;
        int order = value.toInt(&ok);
        if (ok && order > 0) {
            m_filterOrder = order;
            return true;
        }
    }
    
    m_lastError = "无效参数: " + key;
    return false;
}

QVariant MovingAverageFilter::getParameter(const std::string& key) const {
    if (key == "window_size") {
        return static_cast<int>(m_windowSize);
    } else if (key == "cutoff_frequency") {
        return m_cutoffFrequency;
    } else if (key == "filter_order") {
        return m_filterOrder;
    }
    return QVariant();
}

std::map<std::string, QVariant> MovingAverageFilter::getDefaultParameters() const {
    return {
        {"window_size", 5},
        {"cutoff_frequency", 0.5},
        {"filter_order", 1}
    };
}

bool MovingAverageFilter::validateParameters() const {
    return m_windowSize > 0 && m_cutoffFrequency > 0 && m_filterOrder > 0;
}

std::string MovingAverageFilter::getLastError() const {
    return m_lastError;
}

int MovingAverageFilter::getProcessingTime() const {
    return m_processingTime;
}

size_t MovingAverageFilter::getProcessedCount() const {
    return m_processedCount;
}

void MovingAverageFilter::setCutoffFrequency(double freq) {
    m_cutoffFrequency = std::max(0.0, freq);
}

double MovingAverageFilter::getCutoffFrequency() const {
    return m_cutoffFrequency;
}

void MovingAverageFilter::setFilterOrder(int order) {
    m_filterOrder = std::max(1, order);
}

int MovingAverageFilter::getFilterOrder() const {
    return m_filterOrder;
}

double MovingAverageFilter::processSample(double input) {
    if (!m_initialized) {
        return input;
    }
    
    updateBuffer(input);
    
    if (m_buffer.size() < m_windowSize) {
        // 缓冲区未满，返回当前平均值
        return m_sum / m_buffer.size();
    }
    
    return m_sum / m_windowSize;
}

void MovingAverageFilter::updateBuffer(double newValue) {
    if (m_buffer.size() < m_windowSize) {
        // 缓冲区未满，直接添加
        m_buffer.push_back(newValue);
        m_sum += newValue;
    } else {
        // 缓冲区已满，替换最旧的值
        m_sum -= m_buffer[m_currentIndex];
        m_buffer[m_currentIndex] = newValue;
        m_sum += newValue;
        m_currentIndex = (m_currentIndex + 1) % m_windowSize;
    }
}

void MovingAverageFilter::initializeBuffer() {
    m_buffer.clear();
    m_buffer.reserve(m_windowSize);
    m_currentIndex = 0;
    m_sum = 0.0;
}

// ==================== LowPassFilter ====================

LowPassFilter::LowPassFilter() 
    : m_cutoffFrequency(0.1), m_filterOrder(2), m_processingTime(0), m_processedCount(0) {
    calculateCoefficients();
}

LowPassFilter::~LowPassFilter() {
    shutdown();
}

std::string LowPassFilter::getName() const {
    return "LowPassFilter";
}

std::string LowPassFilter::getVersion() const {
    return "1.0.0";
}

std::string LowPassFilter::getDescription() const {
    return "低通滤波插件，用于滤除高频噪声";
}

std::string LowPassFilter::getAuthor() const {
    return "Data Parsing Tool Team";
}

std::vector<std::string> LowPassFilter::getDependencies() const {
    return {};
}

bool LowPassFilter::initialize() {
    try {
        calculateCoefficients();
        m_previousInputs.assign(m_filterOrder + 1, 0.0);
        m_previousOutputs.assign(m_filterOrder + 1, 0.0);
        m_lastError.clear();
        return true;
    } catch (const std::exception& e) {
        m_lastError = std::string("初始化失败: ") + e.what();
        return false;
    }
}

bool LowPassFilter::shutdown() {
    m_previousInputs.clear();
    m_previousOutputs.clear();
    m_coefficientsA.clear();
    m_coefficientsB.clear();
    m_processedCount = 0;
    m_processingTime = 0;
    return true;
}

bool LowPassFilter::isInitialized() const {
    return !m_coefficientsA.empty() && !m_coefficientsB.empty();
}

bool LowPassFilter::processData(std::shared_ptr<DataModel> input, 
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
        
        for (const auto& fieldName : fieldNames) {
            const auto& inputData = input->getDataSeries(fieldName);
            if (inputData.empty()) {
                continue;
            }
            
            std::vector<double> outputData;
            outputData.reserve(inputData.size());
            
            // 应用低通滤波
            for (size_t i = 0; i < inputData.size(); ++i) {
                double inputValue = inputData[i];
                double outputValue = 0.0;
                
                // IIR滤波器实现: y[n] = sum(b[i]*x[n-i]) - sum(a[i]*y[n-i])
                for (size_t j = 0; j < m_coefficientsB.size(); ++j) {
                    if (i >= j) {
                        outputValue += m_coefficientsB[j] * inputData[i - j];
                    }
                }
                
                for (size_t j = 1; j < m_coefficientsA.size(); ++j) {
                    if (i >= j) {
                        outputValue -= m_coefficientsA[j] * outputData[i - j];
                    }
                }
                
                outputData.push_back(outputValue);
            }
            
            output->addDataSeries(fieldName, outputData);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        m_processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        m_processedCount += input->size();
        
        m_lastError.clear();
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = std::string("处理数据失败: ") + e.what();
        return false;
    }
}

bool LowPassFilter::setParameter(const std::string& key, const QVariant& value) {
    if (key == "cutoff_frequency") {
        bool ok;
        double freq = value.toDouble(&ok);
        if (ok && freq > 0 && freq < 1.0) {
            m_cutoffFrequency = freq;
            calculateCoefficients();
            return true;
        }
    } else if (key == "filter_order") {
        bool ok;
        int order = value.toInt(&ok);
        if (ok && order > 0 && order <= 10) {
            m_filterOrder = order;
            calculateCoefficients();
            return true;
        }
    }
    
    m_lastError = "无效参数: " + key;
    return false;
}

QVariant LowPassFilter::getParameter(const std::string& key) const {
    if (key == "cutoff_frequency") {
        return m_cutoffFrequency;
    } else if (key == "filter_order") {
        return m_filterOrder;
    }
    return QVariant();
}

std::map<std::string, QVariant> LowPassFilter::getDefaultParameters() const {
    return {
        {"cutoff_frequency", 0.1},
        {"filter_order", 2}
    };
}

bool LowPassFilter::validateParameters() const {
    return m_cutoffFrequency > 0 && m_cutoffFrequency < 1.0 && 
           m_filterOrder > 0 && m_filterOrder <= 10;
}

std::string LowPassFilter::getLastError() const {
    return m_lastError;
}

int LowPassFilter::getProcessingTime() const {
    return m_processingTime;
}

size_t LowPassFilter::getProcessedCount() const {
    return m_processedCount;
}

void LowPassFilter::setCutoffFrequency(double freq) {
    m_cutoffFrequency = std::max(0.001, std::min(0.999, freq));
    calculateCoefficients();
}

double LowPassFilter::getCutoffFrequency() const {
    return m_cutoffFrequency;
}

void LowPassFilter::setFilterOrder(int order) {
    m_filterOrder = std::max(1, std::min(10, order));
    calculateCoefficients();
}

int LowPassFilter::getFilterOrder() const {
    return m_filterOrder;
}

void LowPassFilter::calculateCoefficients() {
    // 简化的Butterworth低通滤波器系数计算
    // 实际应用中应使用更精确的滤波器设计方法
    
    m_coefficientsA.clear();
    m_coefficientsB.clear();
    
    if (m_filterOrder == 1) {
        // 一阶低通滤波器
        double rc = 1.0 / (2.0 * M_PI * m_cutoffFrequency);
        double dt = 1.0; // 假设采样间隔为1
        double alpha = dt / (rc + dt);
        
        m_coefficientsA = {1.0, alpha - 1.0};
        m_coefficientsB = {alpha, 0.0};
    } else if (m_filterOrder == 2) {
        // 二阶低通滤波器（简化实现）
        double wc = 2.0 * M_PI * m_cutoffFrequency;
        double q = 0.707; // 品质因数
        
        double b0 = wc * wc;
        double b1 = 2.0 * wc * wc;
        double b2 = wc * wc;
        double a0 = 4.0 + 2.0 * wc / q + wc * wc;
        double a1 = 2.0 * wc * wc - 8.0;
        double a2 = 4.0 - 2.0 * wc / q + wc * wc;
        
        m_coefficientsA = {a0, a1 / a0, a2 / a0};
        m_coefficientsB = {b0 / a0, b1 / a0, b2 / a0};
    } else {
        // 高阶滤波器使用简单移动平均作为降级方案
        m_lastError = "高阶滤波器暂未实现，使用移动平均";
        m_coefficientsA = {1.0};
        m_coefficientsB = {1.0 / (m_filterOrder + 1)};
        for (int i = 0; i < m_filterOrder; ++i) {
            m_coefficientsB.push_back(1.0 / (m_filterOrder + 1));
        }
    }
}