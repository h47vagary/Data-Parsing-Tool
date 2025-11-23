#ifndef REALTIMEDATASOURCE_H
#define REALTIMEDATASOURCE_H

#include "DataSource.h"
#include "DataModel.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <ctime>
#include <random>
#include <pthread.h>

class RealTimeDataSource : public DataSource {
public:
    // 实时数据生成模式
    enum class DataMode {
        SineWave,        // 正弦波
        SquareWave,      // 方波
        TriangleWave,    // 三角波
        RandomNoise,     // 随机噪声
        LinearRamp,      // 线性斜坡
        CustomFunction   // 自定义函数
    };
    
    // 实时数据配置
    struct RealTimeConfig {
        DataMode mode;              // 数据生成模式
        double sampleRate;          // 采样率 (Hz)
        double amplitude;           // 幅度
        double frequency;           // 频率 (Hz)
        double offset;              // 偏移量
        double noiseLevel;          // 噪声水平
        int bufferSize;            // 缓冲区大小
        bool autoStart;            // 是否自动开始
        
        RealTimeConfig() 
            : mode(DataMode::SineWave), sampleRate(10.0), amplitude(1.0), 
              frequency(1.0), offset(0.0), noiseLevel(0.0), 
              bufferSize(1000), autoStart(false) {}
    };

    RealTimeDataSource();
    ~RealTimeDataSource();
    
    // DataSource 接口实现
    bool initialize(const std::string& config) override;
    bool start() override;
    void stop() override;
    State getState() const override { return m_state; }
    
    std::vector<double> getData() override;
    bool hasNewData() const override { return m_hasNewData; }
    
    // 实时数据源特定方法
    void setConfig(const RealTimeConfig& config);
    RealTimeConfig getConfig() const { return m_config; }
    
    // 数据流控制
    void pause();
    void resume();
    void setSampleRate(double rate);
    void setAmplitude(double amplitude);
    
    // 自定义数据生成器
    void setCustomDataGenerator(std::function<double(double)> generator);
    
    // 数据访问
    std::shared_ptr<DataModel> getDataModel() const { return m_dataModel; }
    double getCurrentValue() const { return m_currentValue; }
    double getElapsedTime() const;
    
    // 统计信息
    struct RealTimeStats {
        double elapsedTime;
        size_t totalSamples;
        double currentValue;
        double minValue;
        double maxValue;
        double averageValue;
    };
    
    RealTimeStats getStatistics() const;

private:
    // 数据生成函数
    double generateSineWave(double time);
    double generateSquareWave(double time);
    double generateTriangleWave(double time);
    double generateRandomNoise();
    double generateLinearRamp(double time);
    double generateCustomData(double time);
    
    void updateData();
    void addDataPoint(double value);
    void updateStatistics(double value);
    
    // 线程控制
    void dataGenerationThread();
    static void* threadEntry(void* arg);
    
    RealTimeConfig m_config;
    std::shared_ptr<DataModel> m_dataModel;
    State m_state;
    bool m_hasNewData;
    bool m_isPaused;
    
    // 数据生成状态
    double m_currentValue;
    double m_startTime;
    size_t m_sampleCount;
    
    // 统计信息
    mutable pthread_mutex_t m_statsMutex;
    double m_minValue;
    double m_maxValue;
    double m_valueSum;
    
    // 随机数生成器
    std::default_random_engine m_randomEngine;
    std::normal_distribution<double> m_normalDist;
    
    // 自定义数据生成器
    std::function<double(double)> m_customGenerator;
    
    // 线程控制
    pthread_t m_threadId;
    bool m_threadRunning;
    mutable pthread_mutex_t m_threadMutex;
    pthread_cond_t m_threadCond;
};

#endif // REALTIMEDATASOURCE_H