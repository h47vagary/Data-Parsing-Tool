#include "CoreToQtAdapter.h"
#include "data/DataSource.h"
#include "data/DataSourceFactory.h"
#include "plugins/PluginInterface.h"
#include <QDebug>

CoreToQtAdapter::CoreToQtAdapter(QObject* parent) 
    : QObject(parent) 
{
}

CoreToQtAdapter::~CoreToQtAdapter() {
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
    
    if (source && source->initialize(filename.toStdString())) {
        setDataSource(source);
        if (source->start()) {
            qDebug() << "CSV文件加载成功:" << filename;
        }
    }
}

void CoreToQtAdapter::handleDataReady() {
    if (m_dataSource) {
        m_currentData = m_dataSource->getData();
        QVector<double> qtData;
        for (double value : m_currentData) {
            qtData.append(value);
        }
        emit dataReady(qtData);
    }
}

void CoreToQtAdapter::handleError(const std::string& error) {
    emit errorOccurred(QString::fromStdString(error));
}

QVector<double> CoreToQtAdapter::getData() const {
    QVector<double> result;
    for (double value : m_currentData) {
        result.append(value);
    }
    return result;
}