#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QCustomPlot>
#include <QVector>
#include <QMap>
#include <QSharedPointer>
#include <memory>

// 前向声明
class DataModel;
class InteractionHandler;
class ChartManager;

/**
 * @brief 主绘图控件类
 * 
 * 功能特性：
 * - 多曲线显示
 * - 实时数据更新
 * - 交互操作（缩放、拖动、选择）
 * - 工具提示显示
 * - 图例管理
 */
class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);
    ~PlotWidget();

    // === 数据管理 ===
    void setDataModel(QSharedPointer<DataModel> dataModel);
    void addDataSeries(const QString& name, const QVector<double>& xData, 
                      const QVector<double>& yData);
    void addRealTimeData(const QString& seriesName, double x, double y);
    void clearData();
    void clearSeries(const QString& seriesName = QString());
    
    // === 图表配置 ===
    void setTitle(const QString& title);
    void setXAxisLabel(const QString& label);
    void setYAxisLabel(const QString& label);
    void setAxisRange(double xMin, double xMax, double yMin, double yMax);
    void setAutoScale(bool autoScale);
    
    // === 曲线样式 ===
    void setSeriesColor(const QString& seriesName, const QColor& color);
    void setSeriesWidth(const QString& seriesName, double width);
    void setSeriesStyle(const QString& seriesName, Qt::PenStyle style);
    void setSeriesVisible(const QString& seriesName, bool visible);
    
    // === 交互功能 ===
    void enableDrag(bool enabled);
    void enableZoom(bool enabled);
    void enableSelection(bool enabled);
    void enableToolTips(bool enabled);
    void setInteractionEnabled(bool enabled);
    
    // === 导出功能 ===
    bool savePlot(const QString& filename, int width = 800, int height = 600);
    bool copyToClipboard();
    
    // === 状态查询 ===
    QStringList getSeriesNames() const;
    bool hasData() const;
    QRectF getDataRange() const;

public slots:
    void onDataUpdated();
    void onRealTimeDataAdded(const QString& seriesName, double x, double y);
    void onSeriesVisibilityChanged(const QString& seriesName, bool visible);
    void onReplotRequested();

signals:
    void dataPointSelected(const QString& seriesName, double x, double y);
    void viewportChanged(const QRectF& newRange);
    void seriesAdded(const QString& seriesName);
    void seriesRemoved(const QString& seriesName);
    void errorOccurred(const QString& errorMessage);

private slots:
    void onMouseMove(QMouseEvent* event);
    void onMousePress(QMouseEvent* event);
    void onMouseRelease(QMouseEvent* event);
    void onMouseWheel(QWheelEvent* event);
    void onSelectionChanged();
    void onLegendClick(QCPLegend* legend, QCPAbstractLegendItem* item, QMouseEvent* event);

private:
    void setupPlot();
    void setupInteractions();
    void setupLegend();
    void updatePlot();
    void autoScaleAxes();
    void showValueTooltip(double x, double y, const QPoint& screenPos);
    QColor generateColor(int index) const;
    
    QCustomPlot* m_customPlot;
    QSharedPointer<DataModel> m_dataModel;
    QSharedPointer<InteractionHandler> m_interactionHandler;
    QSharedPointer<ChartManager> m_chartManager;
    
    QMap<QString, QCPGraph*> m_graphs;
    QCPItemTracer* m_selectionTracer;
    QCPItemText* m_tooltip;
    QCPItemLine* m_crosshair;
    
    bool m_autoScale;
    bool m_showTooltips;
    bool m_isRealTime;
    
    // 样式配置
    QFont m_titleFont;
    QFont m_axisFont;
    QColor m_backgroundColor;
    QColor m_gridColor;
};

#endif // PLOTWIDGET_H