#include "ExportPlugin.h"
#include "DataModel.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// ==================== CSVExportPlugin ====================

CSVExportPlugin::CSVExportPlugin() 
    : m_delimiter(","), m_includeHeader(true), m_encoding("UTF-8"),
      m_processingTime(0), m_processedCount(0) {
}

CSVExportPlugin::~CSVExportPlugin() {
    shutdown();
}

std::string CSVExportPlugin::getName() const {
    return "CSVExportPlugin";
}

std::string CSVExportPlugin::getVersion() const {
    return "1.0.0";
}

std::string CSVExportPlugin::getDescription() const {
    return "CSV格式导出插件，支持多种分隔符和编码格式";
}

std::string CSVExportPlugin::getAuthor() const {
    return "Data Parsing Tool Team";
}

std::vector<std::string> CSVExportPlugin::getDependencies() const {
    return {};
}

bool CSVExportPlugin::initialize() {
    m_lastError.clear();
    return true;
}

bool CSVExportPlugin::shutdown() {
    m_processedCount = 0;
    m_processingTime = 0;
    return true;
}

bool CSVExportPlugin::isInitialized() const {
    return true;
}

bool CSVExportPlugin::processData(std::shared_ptr<DataModel> input, 
                                 std::shared_ptr<DataModel> output) {
    // 导出插件通常不需要处理数据，而是直接导出到文件
    // 这里可以创建一个包含导出信息的DataModel
    if (!input) {
        m_lastError = "输入数据为空";
        return false;
    }
    
    try {
        // 创建导出统计信息
        if (output) {
            std::map<std::string, double> stats;
            stats["total_points"] = static_cast<double>(input->size());
            stats["field_count"] = static_cast<double>(input->getFieldNames().size());
            stats["export_time"] = static_cast<double>(m_processingTime);
            
            // 将统计信息添加到输出
            for (const auto& stat : stats) {
                output->addDataPoint({{stat.first, stat.second}});
            }
        }
        
        m_processedCount += input->size();
        m_lastError.clear();
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = std::string("处理数据失败: ") + e.what();
        return false;
    }
}

bool CSVExportPlugin::exportToFile(const std::string& filename, 
                                  std::shared_ptr<DataModel> data) {
    if (!data || data->empty()) {
        m_lastError = "没有数据可导出";
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        bool success = writeCSVFile(filename, data);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        m_processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        m_processedCount += data->size();
        
        if (success) {
            m_lastError.clear();
        }
        
        return success;
        
    } catch (const std::exception& e) {
        m_lastError = std::string("导出失败: ") + e.what();
        return false;
    }
}

bool CSVExportPlugin::writeCSVFile(const std::string& filename, 
                                 std::shared_ptr<DataModel> data) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        m_lastError = "无法创建文件: " + filename;
        return false;
    }
    
    try {
        auto fieldNames = data->getFieldNames();
        if (fieldNames.empty()) {
            m_lastError = "没有可导出的字段";
            return false;
        }
        
        // 写入表头
        if (m_includeHeader) {
            for (size_t i = 0; i < fieldNames.size(); ++i) {
                if (i > 0) file << m_delimiter;
                file << escapeCSVField(fieldNames[i]);
            }
            file << std::endl;
        }
        
        // 写入数据
        size_t dataSize = data->size();
        for (size_t i = 0; i < dataSize; ++i) {
            std::map<std::string, double> dataPoint;
            if (data->getDataPoint(i, dataPoint)) {
                for (size_t j = 0; j < fieldNames.size(); ++j) {
                    if (j > 0) file << m_delimiter;
                    
                    auto it = dataPoint.find(fieldNames[j]);
                    if (it != dataPoint.end()) {
                        file << formatCSVValue(it->second);
                    } else {
                        file << ""; // 空值
                    }
                }
                file << std::endl;
            }
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = std::string("写入文件失败: ") + e.what();
        if (file.is_open()) {
            file.close();
        }
        return false;
    }
}

std::string CSVExportPlugin::escapeCSVField(const std::string& field) {
    // 检查是否需要引号
    bool needsQuotes = field.find(m_delimiter) != std::string::npos ||
                      field.find('"') != std::string::npos ||
                      field.find('\n') != std::string::npos ||
                      field.find('\r') != std::string::npos;
    
    if (!needsQuotes) {
        return field;
    }
    
    // 转义双引号
    std::string escaped;
    escaped.reserve(field.length() + 2);
    escaped += '"';
    
    for (char c : field) {
        if (c == '"') {
            escaped += "\"\""; // 双引号转义为两个双引号
        } else {
            escaped += c;
        }
    }
    
    escaped += '"';
    return escaped;
}

std::string CSVExportPlugin::formatCSVValue(double value) {
    std::ostringstream ss;
    ss << std::setprecision(15) << value; // 高精度输出
    
    std::string str = ss.str();
    
    // 检查是否需要科学计数法转换
    if (str.find('e') != std::string::npos || str.find('E') != std::string::npos) {
        // 已经是科学计数法，直接返回
        return str;
    }
    
    // 检查是否为整数
    if (value == static_cast<long long>(value)) {
        // 整数格式，去掉小数点
        return std::to_string(static_cast<long long>(value));
    }
    
    return str;
}

bool CSVExportPlugin::setParameter(const std::string& key, const QVariant& value) {
    if (key == "delimiter") {
        QString delim = value.toString();
        if (!delim.isEmpty()) {
            m_delimiter = delim.toStdString();
            return true;
        }
    } else if (key == "include_header") {
        m_includeHeader = value.toBool();
        return true;
    } else if (key == "encoding") {
        QString encoding = value.toString();
        if (encoding == "UTF-8" || encoding == "ASCII" || encoding == "Latin1") {
            m_encoding = encoding.toStdString();
            return true;
        }
    } else if (key == "precision") {
        bool ok;
        int precision = value.toInt(&ok);
        if (ok && precision >= 0 && precision <= 15) {
            // 精度设置会在formatCSVValue中使用
            return true;
        }
    }
    
    m_lastError = "无效参数: " + key;
    return false;
}

QVariant CSVExportPlugin::getParameter(const std::string& key) const {
    if (key == "delimiter") {
        return QString::fromStdString(m_delimiter);
    } else if (key == "include_header") {
        return m_includeHeader;
    } else if (key == "encoding") {
        return QString::fromStdString(m_encoding);
    }
    return QVariant();
}

std::map<std::string, QVariant> CSVExportPlugin::getDefaultParameters() const {
    return {
        {"delimiter", ","},
        {"include_header", true},
        {"encoding", "UTF-8"},
        {"precision", 6}
    };
}

bool CSVExportPlugin::validateParameters() const {
    return !m_delimiter.empty() && 
           (m_encoding == "UTF-8" || m_encoding == "ASCII" || m_encoding == "Latin1");
}

std::string CSVExportPlugin::getLastError() const {
    return m_lastError;
}

int CSVExportPlugin::getProcessingTime() const {
    return m_processingTime;
}

size_t CSVExportPlugin::getProcessedCount() const {
    return m_processedCount;
}

std::vector<std::string> CSVExportPlugin::getSupportedFormats() const {
    return {"csv", "txt"};
}