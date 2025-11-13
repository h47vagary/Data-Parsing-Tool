#include "DataModel.h"

DataModel::DataModel()
{
}

void DataModel::clear()
{
    x.clear();
    y.clear();
    z.clear();
    a.clear();
    b.clear();
    c.clear();
}

void DataModel::addPoint(double x_val, double y_val, double z_val, double a_val, double b_val, double c_val)
{
    x.push_back(x_val);
    y.push_back(y_val);
    z.push_back(z_val);
    a.push_back(a_val);
    b.push_back(b_val);
    c.push_back(c_val);
}

void DataModel::setData(const std::vector<double> &x_data, const std::vector<double> &y_data, const std::vector<double> &z_data, const std::vector<double> &a_data, const std::vector<double> &b_data, const std::vector<double> &c_data)
{
    x = x_data;
    y = y_data;
    z = z_data;
    a = a_data;
    b = b_data;
    c = c_data;
}
