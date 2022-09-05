#pragma once
#include "Common.h"
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <Eigen/Core>
#include "Quaternion.h"
#include "MathUtility.h"
namespace py=pybind11;
namespace nl=nlohmann;

class PYBIND11_EXPORT MotionState{
    public:
    double time;
    Eigen::Vector3d pos;//parent系での位置
    Eigen::Vector3d vel;//parent系での速度
    Quaternion q;//parent系での姿勢
    Eigen::Vector3d omega;//parent系での角速度
    Quaternion qh;
    double az,el;
    public:
    //constructors & destructor
    MotionState();
    MotionState(const Eigen::Vector3d& pos_,const Eigen::Vector3d& vel_,const Eigen::Vector3d& omega_,const Quaternion& q_,const double& time_);
    MotionState(const Eigen::Vector3d& pos_,const Eigen::Vector3d& vel_,const Eigen::Vector3d& omega_,const Quaternion& q_,const Quaternion& qh_,const double& az_,const double& el_,const double& time_);
    MotionState(const nl::json& j_);
    ~MotionState();
    //(1)相対位置ベクトルの変換
    Eigen::Vector3d relBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    Eigen::Vector3d relPtoB(const Eigen::Vector3d &v) const;//paernt系⇛self系
    //(2)絶対位置ベクトルの変換
    Eigen::Vector3d absBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    Eigen::Vector3d absPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    //(3)速度ベクトルの変換
    Eigen::Vector3d velBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    Eigen::Vector3d velPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    Eigen::Vector3d velBtoP(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//self系⇛parent系
    Eigen::Vector3d velPtoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//parent系⇛self系
    //(4)角速度ベクトルの変換
    Eigen::Vector3d omegaBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    Eigen::Vector3d omegaPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    //(5)局所水平座標系の計算・変換
    virtual void calcQh();
    virtual Eigen::Vector3d relHtoP(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d relPtoH(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d absHtoP(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d absPtoH(const Eigen::Vector3d &v) const;
    //(6)時刻情報を用いた外挿
    virtual MotionState extrapolate(const double& dt) const;
    virtual MotionState extrapolateTo(const double& dstTime) const;
    nl::json to_json() const;//for use from python side
};
void PYBIND11_EXPORT to_json(nl::json& j,const MotionState& m);
void PYBIND11_EXPORT from_json(const nl::json& j,MotionState& m);

void exportMotionState(py::module &m);