#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <memory>

// 前向声明
class QCustomPlot;
class DataModel;
class QCPGraph;

/**
 * @brief 图表管理器类
 * 
 * 负责：
 * - 图表样式管理
 * - 颜色主题管理
 * - 曲线管理
 * - 坐标轴管理
 */
class ChartManager : public QObject {
    Q_OBJECT

public:
    enum Theme {
        ThemeLight,
        ThemeDark,
        ThemeBlue,
        ThemeScientific
    };
    
    struct ChartConfig {
        Theme theme;
        bool showGrid;
        bool showLegend;
        bool antialiasing;
        int refreshRate;
        double lineWidth;
        QFont titleFont;
        QFont axisFont;
        QFont legendFont;
    };
    
    explicit ChartManager(QCustomPlot* plotWidget, QObject* parent = nullptr);
    ~ChartManager();
    
    // === 主题管理 ===
    void setTheme(Theme theme);
    Theme getCurrentTheme() const;
    void applyTheme(Theme theme);
    
    // === 图表配置 ===
    void setChartConfig(const ChartConfig& config);
    ChartConfig getChartConfig() const;
    void setDefaultConfig();
    
    // === 曲线管理 ===
    QCPGraph* addGraph(const QString& name, const QColor& color = Qt::blue);
    bool removeGraph(const QString& name);
    QCPGraph* getGraph(const QString& name) const;
    QStringList getGraphNames() const;
    
    // === 数据更新 ===
    void updateGraphData(const QString& name, 
                        const QVector<double>& xData, 
                        const QVector<double>& yData);
    void addDataPoint(const QString& name, double x, double y);
    void clearGraphData(const QString& name = QString());
    
    // === 样式设置 ===
    void setGraphColor(const QString& name, const QColor& color);
    void setGraphWidth(const QString& name, double width);
    void setGraphStyle(const QString& name, Qt::PenStyle style);
    void setGraphVisible(const QString& name, bool visible);
    
    // === 坐标轴管理 ===
    void setXAxisRange(double min, double max);
    void setYAxisRange(double min, double max);
    void setAxisLabels(const QString& xLabel, const QString& yLabel);
    void setAxisAutoScale(bool autoScale);
    void fitToData();
    
    // === 图例管理 ===
    void setLegendVisible(bool visible);
    void setLegendPosition(Qt::Alignment alignment);
    void updateLegend();
    
    // === 工具方法 ===
    static QColor generateColor(int index, int total = 10);
    static QFont getDefaultFont(int pointSize = 9);
    static QPen createPen(const QColor& color, double width = 2.0, 
                         Qt::PenStyle style = Qt::SolidLine);

signals:
    void graphAdded(const QString& name);
    void graphRemoved(const QString& name);
    void graphUpdated(const QString& name);
    void themeChanged(Theme newTheme);
    void configChanged(const ChartConfig& newConfig);

private:
    void setupDefaultThemes();
    void applyLightTheme();
    void applyDarkTheme();
    void applyBlueTheme();
    void applyScientificTheme();
    void updatePlotStyle();
    
    QCustomPlot* m_plotWidget;
    ChartConfig m_config;
    QMap<QString, QCPGraph*> m_graphs;
    
    // 预定义颜色表
    QVector<QColor> m_colorTable;
    
    // 主题配置
    QMap<Theme, QMap<QString, QVariant>> m_themeConfigs;
};

#endif // CHARTMANAGER_H