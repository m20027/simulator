#pragma once
#include "Common.h"
#include <cmath>
#include <iostream>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace py=pybind11;

namespace util{
    const double Req=6178137.0;
    const double gravity=9.80665;
    double PYBIND11_EXPORT ft2m(const double &bef);
    double PYBIND11_EXPORT m2ft(const double &bef);
    double PYBIND11_EXPORT deg2rad(const double &bef);
    double PYBIND11_EXPORT rad2deg(const double &bef);
    double PYBIND11_EXPORT N2kgf(const double &bef);
    double PYBIND11_EXPORT kgf2N(const double &bef);
    double PYBIND11_EXPORT lb2kg(const double &bef);
    double PYBIND11_EXPORT kg2lb(const double &bef);
    double PYBIND11_EXPORT N2lbf(const double &bef);
    double PYBIND11_EXPORT lbf2N(const double &bef);
    double PYBIND11_EXPORT psi2Pa(const double &bef);
    double PYBIND11_EXPORT Pa2psi(const double &bef);
    double PYBIND11_EXPORT slug2N(const double &bef);
    double PYBIND11_EXPORT N2slug(const double &bef);
    double PYBIND11_EXPORT slugft22kgm2(const double &bef);
    std::vector<double> PYBIND11_EXPORT atmosphere(const double &h);
    double PYBIND11_EXPORT horizon(const double &h);
};

void exportUnits(py::module &m);
