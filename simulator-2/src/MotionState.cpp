#include "MotionState.h"
MotionState::MotionState()
:pos(0,0,0),vel(0,0,0),omega(0,0,0),q(1,0,0,0),qh(1,0,0,0),az(0),el(0),time(0){
}
MotionState::MotionState(const Eigen::Vector3d& pos_,const Eigen::Vector3d& vel_,const Eigen::Vector3d& omega_,const Quaternion& q_,const double& time_)
:pos(pos_),vel(vel_),omega(omega_),q(q_),qh(1,0,0,0),az(0),el(0),time(time_){
    calcQh();
}
MotionState::MotionState(const Eigen::Vector3d& pos_,const Eigen::Vector3d& vel_,const Eigen::Vector3d& omega_,const Quaternion& q_,const Quaternion& qh_,const double& az_,const double& el_,const double& time_)
:pos(pos_),vel(vel_),omega(omega_),q(q_),qh(qh_),az(az_),el(el_),time(time_){
}
MotionState::MotionState(const nl::json& j_)
:MotionState(j_.get<MotionState>()){
}
MotionState::~MotionState(){}
Eigen::Vector3d MotionState::relBtoP(const Eigen::Vector3d &v) const{
    return q.transformVector(v);
}
Eigen::Vector3d MotionState::relPtoB(const Eigen::Vector3d &v) const{
    return q.conjugate().transformVector(v);
}
Eigen::Vector3d MotionState::absBtoP(const Eigen::Vector3d &v) const{
    return q.transformVector(v)+pos;
}
Eigen::Vector3d MotionState::absPtoB(const Eigen::Vector3d &v) const{
    return q.conjugate().transformVector(v-pos);
}
Eigen::Vector3d MotionState::velBtoP(const Eigen::Vector3d &v) const{
    return q.transformVector(v)+vel;
}
Eigen::Vector3d MotionState::velPtoB(const Eigen::Vector3d &v) const{
    return q.conjugate().transformVector(v-vel);
}
Eigen::Vector3d MotionState::velBtoP(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    return q.transformVector(v)+vel+omega.cross(r);
}
Eigen::Vector3d MotionState::velPtoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    return q.conjugate().transformVector(v-vel-omega.cross(r-pos));
}
Eigen::Vector3d MotionState::omegaBtoP(const Eigen::Vector3d &v) const{
    return relBtoP(v)+omega;
}
Eigen::Vector3d MotionState::omegaPtoB(const Eigen::Vector3d &v) const{
    return relPtoB(v-omega);
}
void MotionState::calcQh(){
    Eigen::Vector3d ex=relBtoP(Eigen::Vector3d(1,0,0));
    double xy=sqrt(ex(0)*ex(0)+ex(1)*ex(1));
    double newEl,newAz;
    if(xy==0){
        newEl=ex(2)>0 ? M_PI_2 : -M_PI_2;
        newAz=az;
    }else{
        newEl=atan2(ex(2),xy);
        newAz=atan2(ex(1),ex(0));
    }
    Eigen::Vector3d exh(cos(newAz),sin(newAz),0);
    Eigen::Vector3d eyh,ezh;
    eyh<<sin(newAz),-cos(newAz),0;
    ezh<<0,0,-1;
    qh=Quaternion::fromBasis(exh,eyh,ezh);
    az=newAz;
    el=newEl;
}
Eigen::Vector3d MotionState::relHtoP(const Eigen::Vector3d &v) const{
    return qh.transformVector(v);
}
Eigen::Vector3d MotionState::relPtoH(const Eigen::Vector3d &v) const{
    return qh.conjugate().transformVector(v);
}
Eigen::Vector3d MotionState::absHtoP(const Eigen::Vector3d &v) const{
    return qh.transformVector(v)+pos;
}
Eigen::Vector3d MotionState::absPtoH(const Eigen::Vector3d &v) const{
    return qh.conjugate().transformVector(v-pos);
}
MotionState MotionState::extrapolate(const double &dt) const{
    double w=omega.norm();
    Quaternion qAft;
    if(w*dt>1e-6){
        Quaternion dq=Quaternion::fromAngle(omega/w,w*dt);
        qAft=(dq*q).normalized();
    }else{
        Eigen::VectorXd dq=q.dqdwi()*omega*dt;
        qAft=(q+Quaternion(dq)).normalized();
    }
    MotionState ret=MotionState(
        pos+vel*dt,
        vel,omega,
        qAft,
        Quaternion(1,0,0,0),
        az,el,time+dt);
        ret.calcQh();
        return ret;
}
MotionState MotionState::extrapolateTo(const double &dstTime) const{
    return extrapolate(dstTime-time);
}
void to_json(nl::json& j,const MotionState& m){
    j={
        {"pos",m.pos},
        {"vel",m.vel},
        {"omega",m.omega},
        {"q",m.q},
        {"qh",m.qh},
        {"az",m.az},
        {"el",m.el},
        {"time",m.time}
    };
}
void from_json(const nl::json& j,MotionState& m){
    assert(j.is_object());
    m.pos=j.at("pos");
    m.vel=j.at("vel");
    m.omega=j.at("omega");
    m.q=j.at("q");
    if(j.contains("az")){
        m.az=j.at("az");
        if(j.contains("qh") && j.contains("el")){
            m.el=j.at("el");
            m.qh=j.at("qh");
        }else{
            m.calcQh();
        }
    }else if(j.contains("qh")){
        Quaternion qh_=j.at("qh");
        Eigen::Vector3d ex=qh_.transformVector(Eigen::Vector3d(1,0,0));
        m.az=atan2(ex(1),ex(0));
        m.calcQh();
    }else{
        m.az=0.0;
        m.calcQh();
    }
    m.time=j.at("time");
}
nl::json MotionState::to_json() const{
    return *this;
}

void exportMotionState(py::module &m)
{
    using namespace pybind11::literals;
    py::class_<MotionState>(m,"MotionState").def(py::init())
    .def(py::init<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Quaternion&,const double&>())
    .def(py::init<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Quaternion&,const Quaternion&,const double&,const double&,const double&>())
    .def(py::init<const nl::json&>())
    .def(py::init([](const py::object& obj){return MotionState(obj);}))
    DEF_FUNC(MotionState,relBtoP)
    DEF_FUNC(MotionState,relPtoB)
    DEF_FUNC(MotionState,absBtoP)
    DEF_FUNC(MotionState,absPtoB)
    .def("velBtoP",py::overload_cast<const Eigen::Vector3d&>(&MotionState::velBtoP,py::const_))
    .def("velBtoP",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&MotionState::velBtoP,py::const_))
    .def("velPtoB",py::overload_cast<const Eigen::Vector3d&>(&MotionState::velPtoB,py::const_))
    .def("velPtoB",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&MotionState::velPtoB,py::const_))
    DEF_FUNC(MotionState,omegaBtoP)
    DEF_FUNC(MotionState,omegaPtoB)
    DEF_FUNC(MotionState,calcQh)
    DEF_FUNC(MotionState,relHtoP)
    DEF_FUNC(MotionState,relPtoH)
    DEF_FUNC(MotionState,absHtoP)
    DEF_FUNC(MotionState,absPtoH)
    DEF_FUNC(MotionState,extrapolate)
    DEF_FUNC(MotionState,extrapolateTo)
    DEF_FUNC(MotionState,to_json)
    DEF_READWRITE(MotionState,pos)
    DEF_READWRITE(MotionState,vel)
    DEF_READWRITE(MotionState,q)
    DEF_READWRITE(MotionState,omega)
    DEF_READWRITE(MotionState,qh)
    DEF_READWRITE(MotionState,az)
    DEF_READWRITE(MotionState,el)
    DEF_READWRITE(MotionState,time)
    ;
}