#include "csv_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include <QElapsedTimer>
#include <cmath>

#include "closest_element.h"

CSVParserWindow::CSVParserWindow(QWidget *parent)
{
    std::cout << __FUNCTION__ << std::endl;
    // 创建主窗口和布局
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // 创建QCustomPlot图表
    customPlot = new QCustomPlot(this);
    layout->addWidget(customPlot);
    
    // 设置窗口大小
    setMinimumSize(800, 600);
    setCentralWidget(centralWidget);

    // 启用缩放和拖动功能
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // 保持等比例缩放
    connect(customPlot, &QCustomPlot::afterReplot, this, [this]()
    {
        customPlot->xAxis->setScaleRatio(customPlot->yAxis, 1.0);
    });

    // 鼠标悬停提示
    tooltip_item_ = new QCPItemText(customPlot);
    tooltip_item_->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    tooltip_item_->setLayer("*overlay");
    tooltip_item_->setFont(QFont("Courier New", 9));
    tooltip_item_->setPen(QPen(Qt::black));
    tooltip_item_->setBrush(QBrush(QColor(255,255,225,230)));
    tooltip_item_->setVisible(false);

    // 绑定鼠标移动事件
    connect(customPlot, &QCustomPlot::mouseMove, this, &CSVParserWindow::on_mouse_move);

    // 竖直线
    vertical_line_ = new QCPItemLine(customPlot);
    vertical_line_->setVisible(false);
    vertical_line_->setPen(QPen(Qt::DashLine));
    // 设置类型为绘图坐标
    vertical_line_->start->setType(QCPItemPosition::ptPlotCoords);
    vertical_line_->end->setType(QCPItemPosition::ptPlotCoords);
    // 初始化坐标在 y 轴范围内
    vertical_line_->start->setCoords(0, customPlot->yAxis->range().lower);
    vertical_line_->end->setCoords(0, customPlot->yAxis->range().upper);

    // 窗口缩放时的竖直线重绘
    connect(customPlot->yAxis,
        static_cast<void (QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged),
        this,
        [this](const QCPRange &newRange){
            if(vertical_line_ && vertical_line_->visible())
            {
                double x_val = vertical_line_->start->key();
                vertical_line_->start->setCoords(x_val, newRange.lower);
                vertical_line_->end->setCoords(x_val, newRange.upper);
                customPlot->replot(QCustomPlot::rpQueuedReplot);
            }
        });

    connect(customPlot, &QCustomPlot::mousePress, this, [this](QMouseEvent *event){
        double x_val = customPlot->xAxis->pixelToCoord(event->pos().x());
        vertical_line_->start->setCoords(x_val, customPlot->yAxis->range().lower);
        vertical_line_->end->setCoords(x_val, customPlot->yAxis->range().upper);
        vertical_line_->setVisible(true);
        customPlot->replot();
    });


}

CSVParserWindow::~CSVParserWindow()
{
}

void CSVParserWindow::on_mouse_move(QMouseEvent *event)
{
    if (customPlot->graphCount() == 0)
        return;

    static QElapsedTimer timer;
    static bool firstCall = true;

    // 限制刷新频率，每 10ms 更新一次
    if (firstCall)
    {
        timer.start();
        firstCall = false;
    }
    else if (timer.elapsed() < 10)
        return;
    timer.restart();

    double mouse_x = customPlot->xAxis->pixelToCoord(event->pos().x());
    double mouse_y = customPlot->yAxis->pixelToCoord(event->pos().y());

    // 仅考虑选中曲线
    QCPGraph *graph = nullptr;
    for (int g = 0; g < customPlot->graphCount(); ++g)
    {
        if (customPlot->graph(g)->selected())
        {
            graph = customPlot->graph(g);
            break;
        }
    }
    if (!graph) return;

    int n = graph->data()->size();
    if (n == 0)
        return;

    // 二分查找最接近的横坐标
    int left = 0, right = n - 1;
    while (left <= right)
    {
        int mid = (left + right) / 2;
        double key = graph->data()->at(mid)->key;
        if (key < mouse_x)
            left = mid + 1;
        else
            right = mid - 1;
    }

    int idx1 = qBound(0, left, n - 1);
    int idx0 = qBound(0, left - 1, n - 1);

    auto point0 = graph->data()->at(idx0);
    auto point1 = graph->data()->at(idx1);

    // 计算像素距离
    double dx0 = customPlot->xAxis->coordToPixel(point0->key) - event->pos().x();
    double dy0 = customPlot->yAxis->coordToPixel(point0->value) - event->pos().y();
    double dist0 = std::sqrt(dx0*dx0 + dy0*dy0);

    double dx1 = customPlot->xAxis->coordToPixel(point1->key) - event->pos().x();
    double dy1 = customPlot->yAxis->coordToPixel(point1->value) - event->pos().y();
    double dist1 = std::sqrt(dx1*dx1 + dy1*dy1);

    double closest_x = 0, closest_y = 0;
    bool found = false;

    if (dist0 < dist1 && dist0 < 10) // 10 像素范围
    {
        closest_x = point0->key;
        closest_y = point0->value;
        found = true;
    }
    else if (dist1 < 10)
    {
        closest_x = point1->key;
        closest_y = point1->value;
        found = true;
    }

    if (found)
    {
        tooltip_item_->setText(QString("(%1 , %2)")
                               .arg(closest_x, 0, 'f', 3)
                               .arg(closest_y, 0, 'f', 3));
        tooltip_item_->position->setCoords(closest_x, closest_y);
        tooltip_item_->setVisible(true);
        customPlot->replot(QCustomPlot::rpQueuedReplot);
    }
    else
    {
        tooltip_item_->setVisible(false);
        customPlot->replot(QCustomPlot::rpQueuedReplot);
    }
}

void CSVParserWindow::plotData()
{
    std::cout << __FUNCTION__ << std::endl;

    // 构造横轴: 索引（点数量）
    QVector<double> indices;
    int pointCount = x.size();
    for (int i = 0; i < pointCount; ++i)
        indices.append(i);

    std::cout << __FUNCTION__ << " pointCount: " << pointCount << std::endl;

    // 清除旧图
    customPlot->clearGraphs();

    // X 坐标图
    customPlot->addGraph();
    customPlot->graph(0)->setData(indices, toQVector(x));
    customPlot->graph(0)->setName("X");
    customPlot->graph(0)->setPen(QPen(Qt::blue));

    // Y 坐标图
    customPlot->addGraph();
    customPlot->graph(1)->setData(indices, toQVector(y));
    customPlot->graph(1)->setName("Y");
    customPlot->graph(1)->setPen(QPen(Qt::red));

    // Z 坐标图
    customPlot->addGraph();
    customPlot->graph(2)->setData(indices, toQVector(z));
    customPlot->graph(2)->setName("Z");
    customPlot->graph(2)->setPen(QPen(Qt::green));

    // A 角度图
    customPlot->addGraph();
    customPlot->graph(3)->setData(indices, toQVector(a));
    customPlot->graph(3)->setName("A");
    customPlot->graph(3)->setPen(QPen(Qt::magenta));

    // B 角度图
    customPlot->addGraph();
    customPlot->graph(4)->setData(indices, toQVector(b));
    customPlot->graph(4)->setName("B");
    customPlot->graph(4)->setPen(QPen(Qt::cyan));

    // C 角度图
    customPlot->addGraph();
    customPlot->graph(5)->setData(indices, toQVector(c));
    customPlot->graph(5)->setName("C");
    customPlot->graph(5)->setPen(QPen(Qt::darkYellow));

    // 设置坐标轴标签
    customPlot->xAxis->setLabel("点序号");
    customPlot->yAxis->setLabel("值");

    customPlot->legend->setVisible(true);
    customPlot->legend->setFont(QFont("Helvetica", 9));

    // 自适应缩放
    customPlot->rescaleAxes();

    // 保持X/Y比例一致
    customPlot->xAxis->setScaleRatio(customPlot->yAxis, 1.0);

    customPlot->replot();
}

void CSVParserWindow::loadData(const QString &filename)
{
    std::cout << __FUNCTION__ << std::endl;
    x.clear();
    y.clear();
    z.clear();
    a.clear();
    b.clear();
    c.clear();
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件:" << filename;
        return;
    }
    
    QTextStream in(&file);
    bool firstLine = true;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (firstLine) {
            firstLine = false;
            continue; // 跳过标题行
        }
        
        QStringList fields = line.split(',');
        if (fields.size() >= 6) {
            x.push_back(fields[0].toDouble());
            y.push_back(fields[1].toDouble());
            z.push_back(fields[2].toDouble());
            a.push_back(fields[3].toDouble());
            b.push_back(fields[4].toDouble());
            c.push_back(fields[5].toDouble());
        }
    }
    
    file.close();
}

void CSVParserWindow::loadData(const std::string &filename, bool is_filtering)
{
    std::cout << __FUNCTION__ << std::endl;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    x.clear();
    y.clear();
    z.clear();
    a.clear();
    b.clear();
    c.clear();

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string token;
        double in_x, in_y, in_z, in_a, in_b, in_c;

        // 依次读取6个字段
        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_x)) return;

        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_y)) return;

        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_z)) return;

        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_a)) return;

        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_b)) return;

        if (!std::getline(ss, token, ',')) return;
        if (!parse_double(token, in_c)) return;

        x.push_back(in_x);
        y.push_back(in_y);
        z.push_back(in_z);
        a.push_back(in_a);
        b.push_back(in_b);
        c.push_back(in_c);
    }

    if (is_filtering)
    {
        // std::unique_ptr<IPoseFilter> pose_filter = std::make_unique<MovingAverageFilter>(filter_window_size_);
        // pose_filter->set_filter_param(11);
        // pose_filter->filter_xyzabc(x, y, z, a, b, c);
    }
}

bool CSVParserWindow::parse_double(const std::string &str, double &value)
{
    char* endptr = nullptr;
    value = std::strtod(str.c_str(), &endptr);
    return endptr != str.c_str() && *endptr == '\0';
}

void CSVParserWindow::set_filter_param(int filter_window_size)
{
    filter_window_size_ = filter_window_size;
}

void CSVParserWindow::save_data_to_file(const std::string &filename)
{
    std::cout << __FUNCTION__ << " filename: " << filename << std::endl;
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    file << "x,y,z,A,B,C\n";
    for (size_t i = 0; i < x.size(); ++i)
    {
        file << x[i] << ","
            << y[i] << ","
            << z[i] << ","
            << a[i] << ","
            << b[i] << ","
            << c[i] << "\n";
    }

    file.close();
}
