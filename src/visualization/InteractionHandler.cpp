#include "InteractionHandler.h"
#include <QCustomPlot>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCPGraph>
#include <QApplication>
#include <cmath>

InteractionHandler::InteractionHandler(QCustomPlot* plotWidget, QObject* parent)
    : QObject(parent)
    , m_plotWidget(plotWidget)
    , m_isDragging(false)
    , m_isMeasuring(false)
    , m_currentMode(ModeNone) {
    
    // 默认配置
    m_config.dragEnabled = true;
    m_config.zoomEnabled = true;
    m_config.selectionEnabled = true;
    m_config.tooltipsEnabled = true;
    m_config.zoomFactor = 1.2;
    m_config.zoomModifier = Qt::NoModifier;
    m_config.panModifier = Qt::NoModifier;
    
    setupConnections();
    createInteractionItems();
}

void InteractionHandler::setupConnections() {
    if (!m_plotWidget) return;
    
    // 安装事件过滤器
    m_plotWidget->installEventFilter(this);
    
    // 连接信号
    connect(m_plotWidget, &QCustomPlot::plottableClick, 
            this, &InteractionHandler::onPlottableClick);
}

void InteractionHandler::createInteractionItems() {
    if (!m_plotWidget) return;
    
    // 创建测量线
    m_measureLine = new QCPItemLine(m_plotWidget);
    m_measureLine->setVisible(false);
    m_measureLine->setPen(QPen(Qt::red, 2, Qt::DashLine));
    
    // 创建测量文本
    m_measureText = new QCPItemText(m_plotWidget);
    m_measureText->setVisible(false);
    m_measureText->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_measureText->setTextAlignment(Qt::AlignLeft);
    m_measureText->setFont(QFont("Arial", 10));
    m_measureText->setPen(QPen(Qt::black));
    m_measureText->setBrush(QBrush(QColor(255, 255, 255, 200)));
    
    // 创建十字线
    m_crosshairX = new QCPItemLine(m_plotWidget);
    m_crosshairY = new QCPItemLine(m_plotWidget);
    m_crosshairX->setVisible(false);
    m_crosshairY->setVisible(false);
    m_crosshairX->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    m_crosshairY->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    
    // 创建工具提示
    m_tooltip = new QCPItemText(m_plotWidget);
    m_tooltip->setVisible(false);
    m_tooltip->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_tooltip->setTextAlignment(Qt::AlignLeft);
    m_tooltip->setFont(QFont("Arial", 9));
    m_tooltip->setPen(QPen(Qt::black));
    m_tooltip->setBrush(QBrush(QColor(255, 255, 225, 230)));
}

bool InteractionHandler::eventFilter(QObject* obj, QEvent* event) {
    if (obj != m_plotWidget) {
        return false;
    }
    
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        onMousePress(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseMove:
        onMouseMove(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonRelease:
        onMouseRelease(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonDblClick:
        onMouseDoubleClick(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::Wheel:
        onWheelEvent(static_cast<QWheelEvent*>(event));
        break;
    default:
        break;
    }
    
    return false; // 继续传递事件
}

void InteractionHandler::onMousePress(QMouseEvent* event) {
    if (!m_plotWidget) return;
    
    switch (m_currentMode) {
    case ModeDrag:
        if (event->button() == Qt::LeftButton) {
            m_isDragging = true;
            m_dragStartPos = event->pos();
            m_originalViewport = m_plotWidget->axisRect()->rect();
            QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        }
        break;
        
    case ModeSelect:
        if (event->button() == Qt::LeftButton) {
            m_selectionStart = event->pos();
        }
        break;
        
    case ModeMeasure:
        if (event->button() == Qt::LeftButton) {
            QPointF coord = pixelToCoord(event->pos());
            startMeasurement(coord);
        }
        break;
        
    default:
        break;
    }
}

void InteractionHandler::onMouseMove(QMouseEvent* event) {
    if (!m_plotWidget) return;
    
    // 更新十字线
    updateCrosshair(event->pos());
    
    // 显示工具提示
    if (m_config.tooltipsEnabled) {
        showToolTip(event->pos());
    }
    
    switch (m_currentMode) {
    case ModeDrag:
        if (m_isDragging) {
            handleDrag(event);
        }
        break;
        
    case ModeSelect:
        // 处理选择框绘制
        break;
        
    case ModeMeasure:
        if (m_isMeasuring) {
            QPointF coord = pixelToCoord(event->pos());
            updateMeasurement(coord);
        }
        break;
        
    default:
        break;
    }
}

void InteractionHandler::handleDrag(QMouseEvent* event) {
    if (!m_isDragging) return;
    
    int dx = event->pos().x() - m_dragStartPos.x();
    int dy = event->pos().y() - m_dragStartPos.y();
    
    double xScale = m_plotWidget->xAxis->range().size() / m_plotWidget->axisRect()->width();
    double yScale = m_plotWidget->yAxis->range().size() / m_plotWidget->axisRect()->height();
    
    double xMove = -dx * xScale;
    double yMove = dy * yScale;
    
    m_plotWidget->xAxis->moveRange(xMove);
    m_plotWidget->yAxis->moveRange(yMove);
    m_plotWidget->replot();
    
    emit viewportChanged(m_plotWidget->axisRect()->rect());
}

void InteractionHandler::zoomIn(double factor) {
    if (!m_plotWidget) return;
    
    m_plotWidget->xAxis->scaleRange(1.0 / factor, m_plotWidget->xAxis->range().center());
    m_plotWidget->yAxis->scaleRange(1.0 / factor, m_plotWidget->yAxis->range().center());
    m_plotWidget->replot();
    
    emit viewportChanged(m_plotWidget->axisRect()->rect());
}

void InteractionHandler::resetView() {
    if (!m_plotWidget) return;
    
    m_plotWidget->rescaleAxes();
    m_plotWidget->replot();
    emit viewportChanged(m_plotWidget->axisRect()->rect());
}