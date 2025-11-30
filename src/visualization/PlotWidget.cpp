#include "PlotWidget.h"
#include "ChartManager.h"
#include "InteractionHandler.h"
#include "data/DataModel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <algorithm>
#include <cmath>

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
    , m_autoScale(true)
    , m_showTooltips(true)
    , m_isRealTime(false) {
    
    setupUI();
    setupPlot();
    setupInteractions();
}

PlotWidget::~PlotWidget() {
    // 清理资源
}

void PlotWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建QCustomPlot实例
    m_customPlot = new QCustomPlot(this);
    mainLayout->addWidget(m_customPlot);
    
    // 创建管理器
    m_chartManager = QSharedPointer<ChartManager>(new ChartManager(m_customPlot, this));
    m_interactionHandler = QSharedPointer<InteractionHandler>(new InteractionHandler(m_customPlot, this));
    
    // 设置默认样式
    m_chartManager->setDefaultConfig();
}

void PlotWidget::setupPlot() {
    // 基本配置
    m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_customPlot->axisRect()->setupFullAxesBox(true);
    
    // 坐标轴配置
    m_customPlot->xAxis->setTickLabelFont(m_axisFont);
    m_customPlot->yAxis->setTickLabelFont(m_axisFont);
    m_customPlot->xAxis->setLabelFont(m_axisFont);
    m_customPlot->yAxis->setLabelFont(m_axisFont);
    
    // 图例配置
    m_customPlot->legend->setVisible(true);
    m_customPlot->legend->setFont(QFont("Arial", 9));
    m_customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    
    // 网格配置
    m_customPlot->xAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    m_customPlot->yAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
}

void PlotWidget::setupInteractions() {
    // 连接信号槽
    connect(m_customPlot, &QCustomPlot::mouseMove, this, &PlotWidget::onMouseMove);
    connect(m_customPlot, &QCustomPlot::mousePress, this, &PlotWidget::onMousePress);
    connect(m_customPlot, &QCustomPlot::mouseRelease, this, &PlotWidget::onMouseRelease);
    connect(m_customPlot, &QCustomPlot::mouseWheel, this, &PlotWidget::onMouseWheel);
    connect(m_customPlot, &QCustomPlot::selectionChangedByUser, this, &PlotWidget::onSelectionChanged);
    connect(m_customPlot, &QCustomPlot::legendClick, this, &PlotWidget::onLegendClick);
    
    // 启用交互
    m_interactionHandler->enableDrag(true);
    m_interactionHandler->enableZoom(true);
    m_interactionHandler->enableSelection(true);
    m_interactionHandler->enableTooltips(true);
}

void PlotWidget::setDataModel(QSharedPointer<DataModel> dataModel) {
    if (m_dataModel == dataModel) {
        return;
    }
    
    m_dataModel = dataModel;
    if (!m_dataModel) {
        clearData();
        return;
    }
    
    // 从数据模型加载数据
    updatePlot();
}

void PlotWidget::addDataSeries(const QString& name, const QVector<double>& xData, 
                              const QVector<double>& yData) {
    if (xData.isEmpty() || yData.isEmpty() || xData.size() != yData.size()) {
        emit errorOccurred("数据无效: x和y数据长度不匹配或为空");
        return;
    }
    
    try {
        QCPGraph* graph = m_chartManager->addGraph(name, generateColor(m_graphs.size()));
        m_chartManager->updateGraphData(name, xData, yData);
        m_graphs[name] = graph;
        
        if (m_autoScale) {
            m_chartManager->fitToData();
        }
        
        m_customPlot->replot();
        emit seriesAdded(name);
        
    } catch (const std::exception& e) {
        emit errorOccurred(QString("添加数据系列失败: %1").arg(e.what()));
    }
}

void PlotWidget::addRealTimeData(const QString& seriesName, double x, double y) {
    if (!m_graphs.contains(seriesName)) {
        // 创建新曲线
        QCPGraph* graph = m_chartManager->addGraph(seriesName, generateColor(m_graphs.size()));
        m_graphs[seriesName] = graph;
        emit seriesAdded(seriesName);
    }
    
    m_chartManager->addDataPoint(seriesName, x, y);
    m_isRealTime = true;
    
    // 实时模式下自动滚动
    if (m_autoScale && m_isRealTime) {
        auto range = m_chartManager->getDataRange();
        m_chartManager->setXAxisRange(range.left(), range.left() + 100); // 显示最近100个点
    }
    
    m_customPlot->replot();
}

void PlotWidget::clearData() {
    m_chartManager->clearGraphData();
    m_graphs.clear();
    m_customPlot->replot();
    m_isRealTime = false;
}

void PlotWidget::clearSeries(const QString& seriesName) {
    if (seriesName.isEmpty()) {
        clearData();
        return;
    }
    
    if (m_graphs.contains(seriesName)) {
        m_chartManager->removeGraph(seriesName);
        m_graphs.remove(seriesName);
        m_customPlot->replot();
        emit seriesRemoved(seriesName);
    }
}

void PlotWidget::setTitle(const QString& title) {
    m_customPlot->plotLayout()->insertRow(0);
    m_customPlot->plotLayout()->addElement(0, 0, 
        new QCPTextElement(m_customPlot, title, m_titleFont));
    m_customPlot->replot();
}

void PlotWidget::setAxisLabels(const QString& xLabel, const QString& yLabel) {
    m_customPlot->xAxis->setLabel(xLabel);
    m_customPlot->yAxis->setLabel(yLabel);
    m_customPlot->replot();
}

void PlotWidget::setAutoScale(bool autoScale) {
    m_autoScale = autoScale;
    if (m_autoScale) {
        m_chartManager->fitToData();
    }
}

void PlotWidget::updatePlot() {
    if (!m_dataModel) {
        return;
    }
    
    try {
        // 获取数据模型中的所有字段
        auto fieldNames = m_dataModel->getFieldNames();
        if (fieldNames.empty()) {
            return;
        }
        
        // 假设第一个字段是x轴数据
        QString xFieldName = QString::fromStdString(fieldNames[0]);
        auto xData = m_dataModel->getDataSeries(xFieldName.toStdString());
        
        // 为其他字段创建曲线
        for (size_t i = 1; i < fieldNames.size(); ++i) {
            QString fieldName = QString::fromStdString(fieldNames[i]);
            auto yData = m_dataModel->getDataSeries(fieldName.toStdString());
            
            if (xData.size() == yData.size()) {
                QVector<double> xVec, yVec;
                for (size_t j = 0; j < xData.size(); ++j) {
                    xVec.append(xData[j]);
                    yVec.append(yData[j]);
                }
                addDataSeries(fieldName, xVec, yVec);
            }
        }
        
        if (m_autoScale) {
            m_chartManager->fitToData();
        }
        
        m_customPlot->replot();
        
    } catch (const std::exception& e) {
        emit errorOccurred(QString("更新图表失败: %1").arg(e.what()));
    }
}

bool PlotWidget::savePlot(const QString& filename, int width, int height) {
    try {
        QPixmap pixmap = m_customPlot->toPixmap(width, height);
        return pixmap.save(filename);
    } catch (const std::exception& e) {
        emit errorOccurred(QString("保存图表失败: %1").arg(e.what()));
        return false;
    }
}

// 其他方法实现...