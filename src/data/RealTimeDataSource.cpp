#include "RealTimeDataSource.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

RealTimeDataSource::RealTimeDataSource() 
    : m_state(State::Stopped), m_hasNewData(false), m_isPaused(false),
      m_currentValue(0.0), m_startTime(0.0), m_sampleCount(0),
      m_minValue(0.0), m_maxValue(0.0), m_valueSum(0.0),
      m_threadRunning(false), m_dataModel(std::make_shared<DataModel>()) {
    
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&m_statsMutex, NULL);
    pthread_mutex_init(&m_threadMutex, NULL);
    pthread_cond_init(&m_threadCond, NULL);
    
    // 初始化随机数生成器
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    m_randomEngine.seed(seed);
    m_normalDist = std::normal_distribution<double>(0.0, 1.0);
    
    // 添加默认字段
    m_dataModel->addField("time");
    m_dataModel->addField("value");
}

RealTimeDataSource::~RealTimeDataSource() {
    stop();
    
    pthread_mutex_destroy(&m_statsMutex);
    pthread_mutex_destroy(&m_threadMutex);
    pthread_cond_destroy(&m_threadCond);
}

bool RealTimeDataSource::initialize(const std::string& config) {
    // 配置字符串格式: "mode=SineWave;rate=10;amplitude=1.0;frequency=1.0"
    // 简化实现：使用默认配置
    std::cout << "初始化实时数据源: " << config << std::endl;
    return true;
}

bool RealTimeDataSource::start() {
    if (m_state == State::Running) {
        return true;
    }
    
    // 重置数据模型
    m_dataModel->clear();
    m_dataModel->addField("time");
    m_dataModel->addField("value");
    
    // 重置统计信息
    pthread_mutex_lock(&m_statsMutex);
    m_sampleCount = 0;
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_valueSum = 0.0;
    pthread_mutex_unlock(&m_statsMutex);
    
    // 设置开始时间
    auto now = std::chrono::steady_clock::now();
    m_startTime = std::chrono::duration<double>(now.time_since_epoch()).count();
    
    // 启动数据生成线程
    m_threadRunning = true;
    m_isPaused = false;
    
    if (pthread_create(&m_threadId, NULL, &RealTimeDataSource::threadEntry, this) != 0) {
        std::cerr << "无法创建数据生成线程" << std::endl;
        return false;
    }
    
    m_state = State::Running;
    std::cout << "实时数据源已启动，采样率: " << m_config.sampleRate << " Hz" << std::endl;
    
    return true;
}

void RealTimeDataSource::stop() {
    if (m_state != State::Running) {
        return;
    }
    
    m_threadRunning = false;
    m_isPaused = false;
    
    // 通知线程退出
    pthread_mutex_lock(&m_threadMutex);
    pthread_cond_signal(&m_threadCond);
    pthread_mutex_unlock(&m_threadMutex);
    
    // 等待线程结束
    if (pthread_join(m_threadId, NULL) != 0) {
        std::cerr << "等待线程结束失败" << std::endl;
    }
    
    m_state = State::Stopped;
    m_hasNewData = false;
    
    std::cout << "实时数据源已停止" << std::endl;
}

void RealTimeDataSource::pause() {
    if (m_state == State::Running) {
        m_isPaused = true;
        std::cout << "实时数据源已暂停" << std::endl;
    }
}

void RealTimeDataSource::resume() {
    if (m_state == State::Running && m_isPaused) {
        m_isPaused = false;
        pthread_mutex_lock(&m_threadMutex);
        pthread_cond_signal(&m_threadCond);
        pthread_mutex_unlock(&m_threadMutex);
        std::cout << "实时数据源已恢复" << std::endl;
    }
}

void RealTimeDataSource::setSampleRate(double rate) {
    if (rate > 0 && rate <= 1000) { // 限制采样率范围
        m_config.sampleRate = rate;
    }
}

void RealTimeDataSource::setAmplitude(double amplitude) {
    m_config.amplitude = amplitude;
}

void RealTimeDataSource::setConfig(const RealTimeConfig& config) {
    m_config = config;
}

void RealTimeDataSource::setCustomDataGenerator(std::function<double(double)> generator) {
    m_customGenerator = generator;
    if (generator) {
        m_config.mode = DataMode::CustomFunction;
    }
}

std::vector<double> RealTimeDataSource::getData() {
    m_hasNewData = false;
    // 返回最新的数据值
    return std::vector<double>(1, m_currentValue);
}

double RealTimeDataSource::getElapsedTime() const {
    if (m_state != State::Running) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    double currentTime = std::chrono::duration<double>(now.time_since_epoch()).count();
    return currentTime - m_startTime;
}

void* RealTimeDataSource::threadEntry(void* arg) {
    RealTimeDataSource* source = static_cast<RealTimeDataSource*>(arg);
    source->dataGenerationThread();
    return NULL;
}

void RealTimeDataSource::dataGenerationThread() {
    std::cout << "数据生成线程启动" << std::endl;
    
    auto lastUpdateTime = std::chrono::steady_clock::now();
    double accumulatedTime = 0.0;
    
    while (m_threadRunning) {
        if (m_isPaused) {
            // 暂停状态，等待恢复
            pthread_mutex_lock(&m_threadMutex);
            pthread_cond_wait(&m_threadCond, &m_threadMutex);
            pthread_mutex_unlock(&m_threadMutex);
            lastUpdateTime = std::chrono::steady_clock::now();
            continue;
        }
        
        auto currentTime = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - lastUpdateTime).count();
        lastUpdateTime = currentTime;
        
        accumulatedTime += elapsed;
        double expectedInterval = 1.0 / m_config.sampleRate;
        
        if (accumulatedTime >= expectedInterval) {
            // 生成新数据点
            updateData();
            accumulatedTime = 0.0;
        }
        
        // 休眠以避免过度占用CPU
        int sleepMs = static_cast<int>((expectedInterval - accumulatedTime) * 1000);
        if (sleepMs > 0) {
#ifdef _WIN32
            Sleep(sleepMs);
#else
            usleep(sleepMs * 1000);
#endif
        }
    }
    
    std::cout << "数据生成线程结束" << std::endl;
}

void RealTimeDataSource::updateData() {
    double currentTime = getElapsedTime();
    double value = 0.0;
    
    switch (m_config.mode) {
    case DataMode::SineWave:
        value = generateSineWave(currentTime);
        break;
    case DataMode::SquareWave:
        value = generateSquareWave(currentTime);
        break;
    case DataMode::TriangleWave:
        value = generateTriangleWave(currentTime);
        break;
    case DataMode::RandomNoise:
        value = generateRandomNoise();
        break;
    case DataMode::LinearRamp:
        value = generateLinearRamp(currentTime);
        break;
    case DataMode::CustomFunction:
        value = generateCustomData(currentTime);
        break;
    }
    
    // 添加噪声
    if (m_config.noiseLevel > 0) {
        value += m_normalDist(m_randomEngine) * m_config.noiseLevel;
    }
    
    m_currentValue = value;
    addDataPoint(value);
    updateStatistics(value);
    
    m_hasNewData = true;
    if (m_dataReadyCallback) {
        m_dataReadyCallback();
    }
}

void RealTimeDataSource::addDataPoint(double value) {
    double currentTime = getElapsedTime();
    
    std::map<std::string, double> point;
    point["time"] = currentTime;
    point["value"] = value;
    
    m_dataModel->addDataPoint(point);
    
    // 限制缓冲区大小
    if (m_dataModel->size() > m_config.bufferSize) {
        // 移除旧数据，保留最新的bufferSize个点
        auto subset = m_dataModel->getSubset(1, m_dataModel->size());
        m_dataModel->clear();
        
        auto fieldNames = subset->getFieldNames();
        for (const auto& field : fieldNames) {
            m_dataModel->addDataSeries(field, subset->getDataSeries(field));
        }
    }
}

void RealTimeDataSource::updateStatistics(double value) {
    pthread_mutex_lock(&m_statsMutex);
    
    m_sampleCount++;
    m_valueSum += value;
    
    if (m_sampleCount == 1) {
        m_minValue = value;
        m_maxValue = value;
    } else {
        m_minValue = std::min(m_minValue, value);
        m_maxValue = std::max(m_maxValue, value);
    }
    
    pthread_mutex_unlock(&m_statsMutex);
}

// 数据生成函数实现
double RealTimeDataSource::generateSineWave(double time) {
    return m_config.amplitude * std::sin(2.0 * M_PI * m_config.frequency * time) + m_config.offset;
}

double RealTimeDataSource::generateSquareWave(double time) {
    double period = 1.0 / m_config.frequency;
    double phase = std::fmod(time, period) / period;
    return (phase < 0.5) ? m_config.amplitude + m_config.offset : -m_config.amplitude + m_config.offset;
}

double RealTimeDataSource::generateTriangleWave(double time) {
    double period = 1.0 / m_config.frequency;
    double phase = std::fmod(time, period) / period;
    double value = (phase < 0.5) ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase;
    return m_config.amplitude * value + m_config.offset;
}

double RealTimeDataSource::generateRandomNoise() {
    return m_normalDist(m_randomEngine) * m_config.amplitude + m_config.offset;
}

double RealTimeDataSource::generateLinearRamp(double time) {
    double period = 1.0 / m_config.frequency;
    double phase = std::fmod(time, period) / period;
    return m_config.amplitude * (2.0 * phase - 1.0) + m_config.offset;
}

double RealTimeDataSource::generateCustomData(double time) {
    if (m_customGenerator) {
        return m_customGenerator(time);
    }
    return generateSineWave(time); // 默认回退到正弦波
}

RealTimeDataSource::RealTimeStats RealTimeDataSource::getStatistics() const {
    RealTimeStats stats;
    
    pthread_mutex_lock(&m_statsMutex);
    
    stats.elapsedTime = getElapsedTime();
    stats.totalSamples = m_sampleCount;
    stats.currentValue = m_currentValue;
    stats.minValue = m_minValue;
    stats.maxValue = m_maxValue;
    stats.averageValue = (m_sampleCount > 0) ? m_valueSum / m_sampleCount : 0.0;
    
    pthread_mutex_unlock(&m_statsMutex);
    
    return stats;
}