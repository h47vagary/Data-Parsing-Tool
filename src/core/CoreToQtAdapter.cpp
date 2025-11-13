#include "CoreToQtAdapter.h"
#include "data/DataSource.h"
#include "data/DataSourceFactory.h"
#include "data/DataModel.h"
#include <QDebug>

CoreToQtAdapter::CoreToQtAdapter(QObject* parent) 
    : QObject(parent) 
{
}

CoreToQtAdapter::~CoreToQtAdapter() {
    if (m_dataSource) {
        m_dataSource->stop();
    }
}

void CoreToQtAdapter::setDataSource(std::shared_ptr<DataSource> dataSource) {
    if (m_dataSource) {
        // 断开旧数据源的连接
        m_dataSource->setDataReadyCallback(nullptr);
        m_dataSource->setErrorCallback(nullptr);
    }
    
    m_dataSource = dataSource;
    
    if (m_dataSource) {
        // 连接新数据源的回调
        m_dataSource->setDataReadyCallback([this]() { handleDataReady(); });
        m_dataSource->setErrorCallback([this](const std::string& error) { 
            handleError(error); 
        });
    }
}

void CoreToQtAdapter::loadCSVFile(const QString& filename) {
    auto& factory = DataSourceFactory::getInstance();
    auto source = factory.createCSVSource(filename.toStdString());
    
    if (source) {
        setDataSource(source);
        if (source->start()) {
            qDebug() << "CSV文件加载成功:" << filename;
        } else {
            emit errorOccurred("文件加载失败: " + filename);
        }
    } else {
        emit errorOccurred("创建数据源失败");
    }
}

void CoreToQtAdapter::handleDataReady() {
    if (auto csvSource = std::dynamic_pointer_cast<CSVDataSource>(m_dataSource)) {
        m_currentData = csvSource->getDataModel();
        emit dataReady(m_currentData);
    }
}

void CoreToQtAdapter::handleError(const std::string& error) {
    emit errorOccurred(QString::fromStdString(error));
}

QVector<double> CoreToQtAdapter::getDataAsQVector() const {
    QVector<double> result;
    if (m_currentData) {
        const auto& data = m_currentData->getX(); // 以X数据为例
        for (double value : data) {
            result.append(value);
        }
    }
    return result;
}