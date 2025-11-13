#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <vector>
#include <string>
#include <memory>
#include <functional>

class DataSource {
public:
    enum class State { Stopped, Running, Error };
    
    virtual ~DataSource() = default;
    
    virtual bool initialize(const std::string& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual State getState() const = 0;
    
    virtual std::vector<double> getData() = 0;
    virtual bool hasNewData() const = 0;
    
    // 回调机制替代QT信号槽
    void setDataReadyCallback(std::function<void()> callback) { 
        m_dataReadyCallback = callback; 
    }
    
    void setErrorCallback(std::function<void(const std::string&)> callback) { 
        m_errorCallback = callback; 
    }

protected:
    std::function<void()> m_dataReadyCallback;
    std::function<void(const std::string&)> m_errorCallback;
};

#endif // DATASOURCE_H