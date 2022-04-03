#pragma once
#include "Common.h"
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <nlohmann/json.hpp>
#include "Utility.h"

namespace py=pybind11;
namespace nl=nlohmann;

class PYBIND11_EXPORT Quaternion{
    //Eigen::Quaternionをラップする
    private:
    Eigen::Quaternion<double,Eigen::DontAlign> q;
    public:
    Quaternion();
    Quaternion(const double &w,const Eigen::Vector3d &vec_);
    Quaternion(const double &w,const double &x,const double &y,const double &z);
    Quaternion(const Eigen::Vector4d &v);
    Quaternion(const Eigen::Quaternion<double> &other);
    Quaternion(const Quaternion& other);
    double& w();
    const double& w() const;
    void w(const double &w_);
    double& x();
    const double& x() const;
    void x(const double &x_);
    double& y();
    const double& y() const;
    void y(const double &y_);
    double& z();
    const double& z() const;
    void z(const double &z_);
    const double& wGetterPY();
    void wSetterPY(const double &w_);
    const double& xGetterPY();
    void xSetterPY(const double &x_);
    const double& yGetterPY();
    void ySetterPY(const double &y_);
    const double& zGetterPY();
    void zSetterPY(const double &z_);
    Eigen::Map<Eigen::Vector3d> vec();
    void vec(const Eigen::Vector3d &vec_);
    Eigen::Map<Eigen::Vector3d> vecGetterPY();
    void vecSetterPY(const Eigen::Vector3d &vec_);
    std::string toString() const;
    Eigen::Vector4d toArray() const;
    double norm() const;
    void normalize();
    Quaternion normalized() const;
    Quaternion operator+(const Quaternion &other) const;
    Quaternion operator-(const Quaternion &other) const;
    Quaternion operator-() const;
    Quaternion operator*(const Quaternion &other) const;
    double dot(const Quaternion &other) const;
    Quaternion conjugate() const;
    Eigen::Vector3d transformVector(const Eigen::Vector3d &v) const;
    Eigen::Matrix3d toRotationMatrix() const;
    Eigen::Vector3d toRotationVector() const;
    Quaternion slerp(const double &t,const Quaternion &other) const;
    Eigen::Matrix<double,3,4> dC1dq() const;
    Eigen::Matrix<double,3,4> dC2dq() const;
    Eigen::Matrix<double,3,4> dC3dq() const;
    Eigen::Matrix<double,3,4> dR1dq() const;
    Eigen::Matrix<double,3,4> dR2dq() const;
    Eigen::Matrix<double,3,4> dR3dq() const;
    Eigen::Matrix<double,4,3> dqdwi() const;
    static Quaternion fromAngle(const Eigen::Vector3d &ax,const double &theta);
    static Quaternion fromBasis(const Eigen::Vector3d &ex,const Eigen::Vector3d &ey,const Eigen::Vector3d &ez);
    nl::json to_json() const;//for use from python side
};
void to_json(nl::json& j,const Quaternion& q);
void from_json(const nl::json& j,Quaternion& q);
void exportQuaternion(py::module &m);