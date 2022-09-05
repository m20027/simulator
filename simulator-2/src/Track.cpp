#include "Track.h"
#include "Utility.h"
#include "Units.h"
#include "SimulationManager.h"
#include <boost/uuid/nil_generator.hpp>

Track3D::Track3D(){
    truth=boost::uuids::nil_uuid();
    pos<<0,0,0;
    vel<<0,0,0;
}
Track3D::Track3D(std::weak_ptr<PhysicalAsset> truth_){
    truth=truth_.lock()->uuid;
    if(truth==boost::uuids::nil_uuid()){
        time=0;
        pos<<0,0,0;
        vel<<0,0,0;
    }else{
        time=truth_.lock()->manager->getTime();
        pos=truth_.lock()->posI();
        vel=truth_.lock()->velI();
    }
    buffer=std::vector<Track3D>();
}
Track3D::Track3D(std::weak_ptr<PhysicalAsset> truth_,const Eigen::Vector3d &pos_,const Eigen::Vector3d  &vel_,const double& time_){
    truth=truth_.lock()->uuid;
    time=time_;
    pos=pos_;
    vel=vel_;
    buffer=std::vector<Track3D>();
}
Track3D::Track3D(const boost::uuids::uuid& truth_,const Eigen::Vector3d &pos_,const Eigen::Vector3d  &vel_,const double& time_){
    truth=truth_;
    time=time_;
    pos=pos_;
    vel=vel_;
    buffer=std::vector<Track3D>();
}
Track3D::Track3D(const nl::json& j_)
:Track3D(j_.get<Track3D>()){
}
Track3D::Track3D(const Track3D &other){
    truth=other.truth;
    time=other.time;
    pos=other.pos;
    vel=other.vel;
    for(auto& e:other.buffer){
        buffer.push_back(e);
    }
}
Track3D::~Track3D(){}
Eigen::Vector3d Track3D::posI() const{
    return pos;
}
Eigen::Vector3d Track3D::velI() const{
    return vel;
}
bool Track3D::is_none() const{
    return truth==boost::uuids::nil_uuid();
}
Track3D Track3D::copy() const{
    return Track3D(truth,pos,vel,time);
}
bool Track3D::isSame(const Track3D& other) const{
    if(this->is_none() || other.is_none()){
        return false;
    }
    return truth==other.truth;
}
bool Track3D::isSame(const boost::uuids::uuid& other) const{
    if(this->is_none() || other==boost::uuids::nil_uuid()){
        return false;
    }
    return truth==other;
}
bool Track3D::isSame(std::weak_ptr<Asset> other) const{
    if(this->is_none() || other.expired()){
        return false;
    }
    return truth==(other.lock()->uuid);
}
void Track3D::clearBuffer(){
    buffer.clear();
}
void Track3D::addBuffer(const Track3D& other){
    buffer.push_back(other);
}
void Track3D::merge(){
    if(buffer.size()>0){
        for(auto& e:buffer){
            Track3D extrapolated(e);
            extrapolated.updateByExtrapolation(time-e.time);
            pos+=extrapolated.posI();
            vel+=extrapolated.velI();
        }
        pos/=(buffer.size()+1.0);
        vel/=(buffer.size()+1.0);
        clearBuffer();
    }
}
void Track3D::update(const Track3D& other){
    truth=other.truth;
    pos=other.posI();
    vel=other.velI();
    buffer.clear();
}
void Track3D::updateByExtrapolation(const double& dt){
    pos+=vel*dt;
}
Track3D Track3D::extrapolate(const double& dt){
    return Track3D(truth,pos+vel*dt,vel,time+dt);
}
Track3D Track3D::extrapolateTo(const double& dstTime){
    return extrapolate(dstTime-time);
}
nl::json Track3D::to_json() const{
    return *this;
}

Track2D::Track2D(){
    truth=boost::uuids::nil_uuid();
    dir<<1,0,0;
    origin<<0,0,0;
    omega<<0,0,0;
}
Track2D::Track2D(std::weak_ptr<PhysicalAsset> truth_,const Eigen::Vector3d  &origin_){
    truth=truth_.lock()->uuid;
    if(truth==boost::uuids::nil_uuid()){
        time=0;
        dir<<1,0,0;
        origin=origin_;
        omega<<0,0,0;
    }else{
        time=truth_.lock()->manager->getTime();
        Eigen::Vector3d dr=(truth_.lock()->posI()-origin_);
        double R=dr.norm();
        dir=dr/R;
        origin=origin_;
        omega=dir.cross(truth_.lock()->velI())/R;
    }
    buffer=std::vector<Track2D>();
}
Track2D::Track2D(std::weak_ptr<PhysicalAsset> truth_,const Eigen::Vector3d &dir_,const Eigen::Vector3d  &origin_,const Eigen::Vector3d  &omega_,const double& time_){
    truth=truth_.lock()->uuid;
    time=time_;
    dir=dir_;
    origin=origin_;
    omega=omega_;
    buffer=std::vector<Track2D>();
}
Track2D::Track2D(const boost::uuids::uuid& truth_,const Eigen::Vector3d &dir_,const Eigen::Vector3d  &origin_,const Eigen::Vector3d  &omega_,const double& time_){
    truth=truth_;
    time=time_;
    dir=dir_;
    origin=origin_;
    omega=omega_;
    buffer=std::vector<Track2D>();
}
Track2D::Track2D(const nl::json& j_)
:Track2D(j_.get<Track2D>()){
}
Track2D::Track2D(const Track2D &other){
    truth=other.truth;
    time=other.time;
    dir=other.dir;
    origin=other.origin;
    omega=other.omega;
    for(auto& e:other.buffer){
        buffer.push_back(e);
    }
}
Track2D::~Track2D(){}
Eigen::Vector3d Track2D::dirI() const{
    return dir;
}
Eigen::Vector3d Track2D::originI() const{
    return origin;
}
Eigen::Vector3d Track2D::omegaI() const{
    return omega;
}
bool Track2D::is_none() const{
    return truth==boost::uuids::nil_uuid();
}
Track2D Track2D::copy() const{
    return Track2D(truth,dir,origin,omega,time);
}
bool Track2D::isSame(const Track2D& other) const{
    if(this->is_none() || other.is_none()){
        return false;
    }
    return truth==other.truth;
}
bool Track2D::isSame(const boost::uuids::uuid& other) const{
    if(this->is_none() || other==boost::uuids::nil_uuid()){
        return false;
    }
    return truth==other;
}
bool Track2D::isSame(std::weak_ptr<Asset> other) const{
    if(this->is_none() || other.expired()){
        return false;
    }
    return truth==(other.lock()->uuid);
}
void Track2D::clearBuffer(){
    buffer.clear();
}
void Track2D::addBuffer(const Track2D& other){
    buffer.push_back(other);
}
void Track2D::merge(){
    if(buffer.size()>0){
        for(auto& e:buffer){
            Track2D extrapolated(e);
            extrapolated.updateByExtrapolation(time-e.time);
            dir+=extrapolated.dirI();
            origin+=extrapolated.originI();
            omega+=extrapolated.omegaI();
        }
        dir=(dir/(buffer.size()+1.0)).normalized();
        origin/=(buffer.size()+1.0);
        omega/=(buffer.size()+1.0);
        clearBuffer();
    }
}
void Track2D::update(const Track2D& other){
    truth=other.truth;
    time=other.time;
    dir=other.dirI();
    origin=other.originI();
    omega=other.omegaI();
    buffer.clear();
}
void Track2D::updateByExtrapolation(const double& dt){
    double w=omega.norm();   
    if(w>0){
        Quaternion q=Quaternion::fromAngle(omega/w,w*dt);
        dir=q.transformVector(dir);
    }
}
Track2D Track2D::extrapolate(const double& dt){
    double w=omega.norm();
    if(w>0){
        Quaternion q=Quaternion::fromAngle(omega/w,w*dt);
        return Track2D(truth,q.transformVector(dir),origin,omega,time+dt);
    }else{
        return Track2D(truth,dir,origin,omega,time+dt);
    }
}
Track2D Track2D::extrapolateTo(const double& dstTime){
    return extrapolate(dstTime-time);
}
nl::json Track2D::to_json() const{
    return *this;
}

void exportTrack(py::module& m)
{
    using namespace pybind11::literals;
    py::class_<Track3D,Track3DWrap<>,std::shared_ptr<Track3D>>(m,"Track3D")
    .def(py::init<>())
    .def(py::init<std::weak_ptr<PhysicalAsset>>())
    .def(py::init<std::weak_ptr<PhysicalAsset>,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>())
    .def(py::init<const boost::uuids::uuid&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>())
    .def(py::init<const nl::json&>())
    .def(py::init([](const py::object& obj){return Track3D(obj);}))
    DEF_FUNC(Track3D,posI)
    DEF_FUNC(Track3D,velI)
    DEF_FUNC(Track3D,is_none)
    DEF_FUNC(Track3D,copy)
    .def("isSame",py::overload_cast<const Track3D&>(&Track3D::isSame,py::const_))
    .def("isSame",py::overload_cast<const boost::uuids::uuid&>(&Track3D::isSame,py::const_))
    .def("isSame",py::overload_cast<const std::weak_ptr<Asset>>(&Track3D::isSame,py::const_))
    DEF_FUNC(Track3D,clearBuffer)
    DEF_FUNC(Track3D,addBuffer)
    DEF_FUNC(Track3D,merge)
    DEF_FUNC(Track3D,update)
    DEF_FUNC(Track3D,updateByExtrapolation)
    DEF_FUNC(Track3D,extrapolate)
    DEF_FUNC(Track3D,extrapolateTo)
    DEF_FUNC(Track3D,to_json)
    DEF_READWRITE(Track3D,truth)
    DEF_READWRITE(Track3D,time)
    DEF_READWRITE(Track3D,pos)
    DEF_READWRITE(Track3D,vel)
    DEF_READWRITE(Track3D,buffer)
    ;
    py::class_<Track2D,Track2DWrap<>,std::shared_ptr<Track2D>>(m,"Track2D")
    .def(py::init<>())
    .def(py::init<std::weak_ptr<PhysicalAsset>,const Eigen::Vector3d&>())
    .def(py::init<std::weak_ptr<PhysicalAsset>,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>())
    .def(py::init<const boost::uuids::uuid&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>())
    .def(py::init<const nl::json&>())
    .def(py::init([](const py::object& obj){return Track2D(obj);}))
    DEF_FUNC(Track2D,dirI)
    DEF_FUNC(Track2D,originI)
    DEF_FUNC(Track2D,omegaI)
    DEF_FUNC(Track2D,is_none)
    DEF_FUNC(Track2D,copy)
    .def("isSame",py::overload_cast<const Track2D&>(&Track2D::isSame,py::const_))
    .def("isSame",py::overload_cast<const boost::uuids::uuid&>(&Track2D::isSame,py::const_))
    .def("isSame",py::overload_cast<const std::weak_ptr<Asset>>(&Track2D::isSame,py::const_))
    DEF_FUNC(Track2D,clearBuffer)
    DEF_FUNC(Track2D,addBuffer)
    DEF_FUNC(Track2D,merge)
    DEF_FUNC(Track2D,update)
    DEF_FUNC(Track2D,updateByExtrapolation)
    DEF_FUNC(Track2D,extrapolate)
    DEF_FUNC(Track2D,extrapolateTo)
    DEF_FUNC(Track2D,to_json)
    DEF_READWRITE(Track2D,truth)
    DEF_READWRITE(Track2D,time)
    DEF_READWRITE(Track2D,dir)
    DEF_READWRITE(Track2D,origin)
    DEF_READWRITE(Track2D,omega)
    DEF_READWRITE(Track2D,buffer)
    ;
}