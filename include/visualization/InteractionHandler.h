#ifndef INTERACTIONHANDLER_H
#define INTERACTIONHANDLER_H

#include <QObject>
#include <QPoint>
#include <QCursor>

// 前向声明
class QCustomPlot;
class QMouseEvent;
class QWheelEvent;

/**
 * @brief 交互处理类
 * 
 * 处理用户交互：
 * - 鼠标拖动
 * - 缩放操作
 * - 选择操作
 * - 工具提示
 */
class InteractionHandler : public QObject {
    Q_OBJECT

public:
    enum InteractionMode {
        ModeNone,
        ModeDrag,
        ModeZoom,
        ModeSelect,
        ModeMeasure
    };
    
    struct InteractionConfig {
        bool dragEnabled;
        bool zoomEnabled;
        bool selectionEnabled;
        bool tooltipsEnabled;
        double zoomFactor;
        Qt::KeyboardModifiers zoomModifier;
        Qt::KeyboardModifiers panModifier;
    };
    
    explicit InteractionHandler(QCustomPlot* plotWidget, QObject* parent = nullptr);
    ~InteractionHandler();
    
    // === 交互模式设置 ===
    void setInteractionMode(InteractionMode mode);
    InteractionMode getInteractionMode() const;
    void enableDrag(bool enabled);
    void enableZoom(bool enabled);
    void enableSelection(bool enabled);
    void enableTooltips(bool enabled);
    void setConfig(const InteractionConfig& config);
    
    // === 视图控制 ===
    void zoomIn(double factor = 1.2);
    void zoomOut(double factor = 1.2);
    void zoomToRect(const QRectF& rect);
    void pan(double dx, double dy);
    void resetView();
    void fitToData();
    
    // === 选择操作 ===
    QVector<QCPGraph*> getSelectedGraphs() const;
    void clearSelection();
    void selectGraph(QCPGraph* graph, bool selected = true);
    
    // === 测量工具 ===
    void startMeasurement(const QPointF& startPoint);
    void updateMeasurement(const QPointF& currentPoint);
    void endMeasurement();
    QLineF getMeasurementLine() const;
    double getMeasurementDistance() const;

signals:
    void viewportChanged(const QRectF& newViewport);
    void dataPointSelected(double x, double y, QCPGraph* graph);
    void selectionChanged(const QVector<QCPGraph*>& selectedGraphs);
    void measurementCompleted(double distance, const QPointF& start, const QPointF& end);
    void interactionModeChanged(InteractionMode newMode);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onMousePress(QMouseEvent* event);
    void onMouseMove(QMouseEvent* event);
    void onMouseRelease(QMouseEvent* event);
    void onMouseDoubleClick(QMouseEvent* event);
    void onWheelEvent(QWheelEvent* event);
    void onPlottableClick(QCPAbstractPlottable* plottable, QMouseEvent* event);

private:
    void setupConnections();
    void handleDrag(QMouseEvent* event);
    void handleZoom(QWheelEvent* event);
    void handleSelection(QMouseEvent* event);
    void handleMeasurement(QMouseEvent* event);
    void showToolTip(const QPoint& pos);
    void updateCrosshair(const QPoint& pos);
    QPointF pixelToCoord(const QPoint& pixel) const;
    QPoint coordToPixel(const QPointF& coord) const;
    
    QCustomPlot* m_plotWidget;
    InteractionConfig m_config;
    InteractionMode m_currentMode;
    
    // 拖动状态
    bool m_isDragging;
    QPoint m_dragStartPos;
    QRectF m_originalViewport;
    
    // 选择状态
    QPoint m_selectionStart;
    QRect m_selectionRect;
    
    // 测量状态
    bool m_isMeasuring;
    QPointF m_measureStart;
    QPointF m_measureEnd;
    QCPItemLine* m_measureLine;
    QCPItemText* m_measureText;
    
    // 工具提示
    QCPItemText* m_tooltip;
    QCPItemLine* m_crosshairX;
    QCPItemLine* m_crosshairY;
};

#endif // INTERACTIONHANDLER_H