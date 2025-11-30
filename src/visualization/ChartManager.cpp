#include "ChartManager.h"
#include <QCustomPlot>
#include <QCPGraph>
#include <QCPLegend>
#include <QCPAxisRect>
#include <algorithm>
#include <cmath>

ChartManager::ChartManager(QCustomPlot* plotWidget, QObject* parent)
    : QObject(parent)
    , m_plotWidget(plotWidget) {
    
    setupDefaultThemes();
    setDefaultConfig();
}

void ChartManager::setupDefaultThemes() {
    // 浅色主题
    m_themeConfigs[ThemeLight] = {
        {"background", QColor(255, 255, 255)},
        {"foreground", QColor(0, 0, 0)},
        {"grid", QColor(200, 200, 200)},
        {"text", QColor(0, 0, 0)}
    };
    
    // 深色主题
    m_themeConfigs[ThemeDark] = {
        {"background", QColor(50, 50, 50)},
        {"foreground", QColor(255, 255, 255)},
        {"grid", QColor(100, 100, 100)},
        {"text", QColor(255, 255, 255)}
    };
    
    // 蓝色主题
    m_themeConfigs[ThemeBlue] = {
        {"background", QColor(240, 245, 255)},
        {"foreground", QColor(0, 0, 139)},
        {"grid", QColor(200, 220, 255)},
        {"text", QColor(0, 0, 139)}
    };
    
    // 科学主题
    m_themeConfigs[ThemeScientific] = {
        {"background", QColor(255, 255, 255)},
        {"foreground", QColor(0, 0, 0)},
        {"grid", QColor(220, 220, 220)},
        {"text", QColor(0, 0, 0)}
    };
}

void ChartManager::setTheme(Theme theme) {
    if (m_config.theme == theme) {
        return;
    }
    
    m_config.theme = theme;
    applyTheme(theme);
    emit themeChanged(theme);
}

void ChartManager::applyTheme(Theme theme) {
    if (!m_plotWidget) return;
    
    auto themeConfig = m_themeConfigs.value(theme);
    if (themeConfig.isEmpty()) {
        themeConfig = m_themeConfigs[ThemeLight];
    }
    
    QColor bgColor = themeConfig["background"].value<QColor>();
    QColor fgColor = themeConfig["foreground"].value<QColor>();
    QColor gridColor = themeConfig["grid"].value<QColor>();
    QColor textColor = themeConfig["text"].value<QColor>();
    
    m_plotWidget->setBackground(bgColor);
    
    // 坐标轴样式
    for (auto axis : {m_plotWidget->xAxis, m_plotWidget->yAxis, 
                     m_plotWidget->xAxis2, m_plotWidget->yAxis2}) {
        axis->setBasePen(QPen(fgColor));
        axis->setTickPen(QPen(fgColor));
        axis->setSubTickPen(QPen(fgColor));
        axis->setTickLabelColor(textColor);
        axis->setLabelColor(textColor);
        axis->grid()->setPen(QPen(gridColor, 0, Qt::DotLine));
    }
    
    // 图例样式
    m_plotWidget->legend->setTextColor(textColor);
    m_plotWidget->legend->setBrush(QBrush(bgColor));
    m_plotWidget->legend->setBorderPen(QPen(fgColor));
    
    m_plotWidget->replot();
}

QCPGraph* ChartManager::addGraph(const QString& name, const QColor& color) {
    if (m_graphs.contains(name)) {
        return m_graphs[name];
    }
    
    QCPGraph* graph = m_plotWidget->addGraph();
    graph->setName(name);
    graph->setPen(createPen(color, m_config.lineWidth));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
    
    m_graphs[name] = graph;
    emit graphAdded(name);
    
    return graph;
}

void ChartManager::updateGraphData(const QString& name, 
                                 const QVector<double>& xData, 
                                 const QVector<double>& yData) {
    if (!m_graphs.contains(name)) {
        return;
    }
    
    QCPGraph* graph = m_graphs[name];
    graph->setData(xData, yData);
    emit graphUpdated(name);
}

void ChartManager::fitToData() {
    if (!m_plotWidget) return;
    
    m_plotWidget->rescaleAxes();
    
    // 添加一些边距
    auto xRange = m_plotWidget->xAxis->range();
    auto yRange = m_plotWidget->yAxis->range();
    
    double xMargin = (xRange.upper - xRange.lower) * 0.05;
    double yMargin = (yRange.upper - yRange.lower) * 0.05;
    
    m_plotWidget->xAxis->setRange(xRange.lower - xMargin, xRange.upper + xMargin);
    m_plotWidget->yAxis->setRange(yRange.lower - yMargin, yRange.upper + yMargin);
    
    m_plotWidget->replot();
}

QColor ChartManager::generateColor(int index, int total) {
    static const QVector<QColor> defaultColors = {
        QColor(31, 119, 180),   // 蓝色
        QColor(255, 127, 14),   // 橙色
        QColor(44, 160, 44),    // 绿色
        QColor(214, 39, 40),    // 红色
        QColor(148, 103, 189),  // 紫色
        QColor(140, 86, 75),    // 棕色
        QColor(227, 119, 194),  // 粉色
        QColor(127, 127, 127),  // 灰色
        QColor(188, 189, 34),   // 黄色
        QColor(23, 190, 207)    // 青色
    };
    
    if (index < defaultColors.size()) {
        return defaultColors[index];
    }
    
    // 生成新颜色
    float hue = fmod(index * 0.618033988749895f, 1.0f); // 黄金比例分布
    return QColor::fromHsvF(hue, 0.8f, 0.9f);
}