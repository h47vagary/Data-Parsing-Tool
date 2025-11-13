#pragma once

#include <vector>
#include <string>
#include <memory>


class DataModel {
public:
    DataModel();

    // 数据访问
    const std::vector<double>& getX() const { return x; }
    const std::vector<double>& getY() const { return y; }
    const std::vector<double>& getZ() const { return z; }
    const std::vector<double>& getA() const { return a; }
    const std::vector<double>& getB() const { return b; }
    const std::vector<double>& getC() const { return c; }

    // 数据操作
    void clear();
    void addPoint(double x_val, double y_val, double z_val,
                  double a_val, double b_val, double c_val);
    size_t size() const { return x.size(); }
    bool empty() const { return x.empty(); }

    // 数据操作
    void setData(const std::vector<double>& x_data,
                 const std::vector<double>& y_data,
                 const std::vector<double>& z_data,
                 const std::vector<double>& a_data,
                 const std::vector<double>& b_data,
                 const std::vector<double>& c_data);

private:
    std::vector<double> x, y, z;
    std::vector<double> a, b, c;
};