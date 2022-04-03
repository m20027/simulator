#include "PhysicalAsset.h"
#include "Utility.h"
#include "SimulationManager.h"
#include "Controller.h"
#include "Agent.h"
using namespace util;
PhysicalAsset::PhysicalAsset(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Asset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    isAlive_=true;
    manager=instanceConfig.at("manager");
    fullName=instanceConfig.at("fullName").get<std::string>();
    if(fullName.find("/")>=0){
        team=fullName.substr(0,fullName.find("/"));
        group=fullName.substr(0,fullName.rfind("/"));
        name=fullName.substr(fullName.rfind("/")+1);
    }else{
        team=group=name=fullName;
    }
    try{
        parent=instanceConfig.at("parent");
    }catch(...){
        //
    }
    try{
        isBoundToParent=instanceConfig.at("isBound").get<bool>();
    }catch(...){
        isBoundToParent=false;
    }
    if(parent.expired()){
        isBoundToParent=false;
    }
    pos_prev=motion.pos;
    vel_prev=motion.vel;
    motion.time=manager->getTime();
    observables["motion"]=motion;
    observables["isAlive"]=isAlive();
}
PhysicalAsset::~PhysicalAsset(){}
bool PhysicalAsset::isAlive() const{
    return isAlive_;
}
std::string PhysicalAsset::getTeam() const{
    return team;
}
std::string PhysicalAsset::getGroup() const{
    return group;
}
std::string PhysicalAsset::getFullName() const{
    return fullName;
}
std::string PhysicalAsset::getName() const{
    return name;
}
void PhysicalAsset::setAgent(std::weak_ptr<Agent> agent_,const std::string &port_){
    hasAgent=true;
    agent=agent_;
    port=port_;
}
void PhysicalAsset::makeChildren(){
}
void PhysicalAsset::validate(){

}
void PhysicalAsset::perceive(bool inReset){
    if(parent.expired() || !isBoundToParent){
        observables["motion"]=motion;
    }else{
        MotionState asInertial(posI(),velI(),omegaI(),qI(),motion.time);
        observables["motion"]=asInertial;
    }
    observables["isAlive"]=isAlive();
}
void PhysicalAsset::control(){
}
void PhysicalAsset::behave(){
    motion.time=manager->getTime()+manager->getBaseTimeStep();
}
void PhysicalAsset::kill(){
    isAlive_=false;
    observables={
        {"isAlive",false}
    };
    commands=nl::json::object();
}
Eigen::Vector3d PhysicalAsset::relBtoI(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return relBtoP(v);
    }else{
        return parent.lock()->relBtoI(relBtoP(v));
    }
}
Eigen::Vector3d PhysicalAsset::relItoB(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return relPtoB(v);
    }else{
        return parent.lock()->relItoB(v);
    }
}
Eigen::Vector3d PhysicalAsset::relBtoP(const Eigen::Vector3d &v) const{
    return motion.relBtoP(v);
}
Eigen::Vector3d PhysicalAsset::relPtoB(const Eigen::Vector3d &v) const{
    return motion.relPtoB(v);
}
Eigen::Vector3d PhysicalAsset::absBtoI(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return absBtoP(v);
    }else{
        return parent.lock()->absBtoI(absBtoP(v));
    }
}
Eigen::Vector3d PhysicalAsset::absItoB(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return absPtoB(v);
    }else{
        return absPtoB(parent.lock()->absItoB(v));
    }
}
Eigen::Vector3d PhysicalAsset::absBtoP(const Eigen::Vector3d &v) const{
    return motion.absBtoP(v);
}
Eigen::Vector3d PhysicalAsset::absPtoB(const Eigen::Vector3d &v) const{
    return motion.absPtoB(v);
}
Eigen::Vector3d PhysicalAsset::velBtoI(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return velBtoP(v);
    }else{
        return parent.lock()->velBtoP(velBtoP(v),motion.pos);
    }
}
Eigen::Vector3d PhysicalAsset::velItoB(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return velPtoB(v);
    }else{
        return velPtoB(parent.lock()->velItoB(v,posI()));
    }
}
Eigen::Vector3d PhysicalAsset::velBtoP(const Eigen::Vector3d &v) const{
    return motion.velBtoP(v);
}
Eigen::Vector3d PhysicalAsset::velPtoB(const Eigen::Vector3d &v) const{
    return motion.velPtoB(v);
}
Eigen::Vector3d PhysicalAsset::velBtoI(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    if(parent.expired() || !isBoundToParent){
        return velBtoP(v,r);
    }else{
        return parent.lock()->velBtoP(velBtoP(v,r),absBtoP(r));
    }
}
Eigen::Vector3d PhysicalAsset::velItoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    if(parent.expired() || !isBoundToParent){
        return velPtoB(v,r);
    }else{
        return velPtoB(
            parent.lock()->velItoB(v,r),
            parent.lock()->absItoB(r)
        );
    }
}
Eigen::Vector3d PhysicalAsset::velBtoP(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    return motion.velBtoP(v,r);
}
Eigen::Vector3d PhysicalAsset::velPtoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const{
    return motion.velPtoB(v,r);
}
Eigen::Vector3d PhysicalAsset::omegaBtoI(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return omegaBtoP(v);
    }else{
        return parent.lock()->omegaBtoI(omegaBtoP(v));
    }
}
Eigen::Vector3d PhysicalAsset::omegaItoB(const Eigen::Vector3d &v) const{
    if(parent.expired() || !isBoundToParent){
        return omegaPtoB(v);
    }else{
        return omegaPtoB(parent.lock()->omegaItoB(v));
    }
}
Eigen::Vector3d PhysicalAsset::omegaBtoP(const Eigen::Vector3d &v) const{
    return motion.omegaBtoP(v);
}
Eigen::Vector3d PhysicalAsset::omegaPtoB(const Eigen::Vector3d &v) const{
    return motion.omegaPtoB(v);
}
Eigen::Vector3d PhysicalAsset::posI() const{
    if(parent.expired() || !isBoundToParent){
        return motion.pos;
    }else{
        return parent.lock()->absBtoI(motion.pos);
    }
}
Eigen::Vector3d PhysicalAsset::posP() const{
    return motion.pos;
}
Eigen::Vector3d PhysicalAsset::velI() const{
    if(parent.expired() || !isBoundToParent){
        return motion.vel;
    }else{
        return parent.lock()->velBtoI(motion.vel,motion.pos);
    }
}
Eigen::Vector3d PhysicalAsset::velP() const{
    return motion.vel;
}
Quaternion PhysicalAsset::qI() const{
    if(parent.expired() || !isBoundToParent){
        return motion.q;
    }else{
        return parent.lock()->motion.q*motion.q;
    }
}
Quaternion PhysicalAsset::qP() const{
    return motion.q;
}
Eigen::Vector3d PhysicalAsset::omegaI() const{
    if(parent.expired() || !isBoundToParent){
        return motion.omega;
    }else{
        return parent.lock()->omegaBtoI(motion.omega);
    }
}
Eigen::Vector3d PhysicalAsset::omegaP() const{
    return motion.omega;
}
std::shared_ptr<AssetAccessor> PhysicalAsset::getAccessor(){
    if(!accessor){
        accessor=std::make_shared<PhysicalAssetAccessor>(getShared<PhysicalAsset>(this->shared_from_this()));
    }
    return accessor;
}
PhysicalAssetAccessor::PhysicalAssetAccessor(std::shared_ptr<PhysicalAsset> a)
:AssetAccessor(a){
    asset=a;
}
PhysicalAssetAccessor::~PhysicalAssetAccessor(){
}

void exportPhysicalAsset(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(PhysicalAsset)
    DEF_FUNC(PhysicalAsset,isAlive)
    DEF_FUNC(PhysicalAsset,getTeam)
    DEF_FUNC(PhysicalAsset,getGroup)
    DEF_FUNC(PhysicalAsset,getFullName)
    DEF_FUNC(PhysicalAsset,getName)
    DEF_FUNC(PhysicalAsset,makeChildren)
    DEF_FUNC(PhysicalAsset,validate)
    DEF_FUNC(PhysicalAsset,perceive)
    DEF_FUNC(PhysicalAsset,control)
    DEF_FUNC(PhysicalAsset,behave)
    DEF_FUNC(PhysicalAsset,kill)
    DEF_FUNC(PhysicalAsset,relBtoI)
    DEF_FUNC(PhysicalAsset,relItoB)
    DEF_FUNC(PhysicalAsset,relBtoP)
    DEF_FUNC(PhysicalAsset,relPtoB)
    DEF_FUNC(PhysicalAsset,absBtoI)
    DEF_FUNC(PhysicalAsset,absItoB)
    DEF_FUNC(PhysicalAsset,absBtoP)
    DEF_FUNC(PhysicalAsset,absPtoB)
    .def("velBtoI",py::overload_cast<const Eigen::Vector3d&>(&PhysicalAsset::velBtoI,py::const_))
    .def("velBtoI",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&PhysicalAsset::velBtoI,py::const_))
    .def("velItoB",py::overload_cast<const Eigen::Vector3d&>(&PhysicalAsset::velItoB,py::const_))
    .def("velItoB",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&PhysicalAsset::velItoB,py::const_))
    .def("velBtoP",py::overload_cast<const Eigen::Vector3d&>(&PhysicalAsset::velBtoP,py::const_))
    .def("velBtoP",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&PhysicalAsset::velBtoP,py::const_))
    .def("velPtoB",py::overload_cast<const Eigen::Vector3d&>(&PhysicalAsset::velPtoB,py::const_))
    .def("velPtoB",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&>(&PhysicalAsset::velPtoB,py::const_))
    DEF_FUNC(PhysicalAsset,omegaBtoI)
    DEF_FUNC(PhysicalAsset,omegaItoB)
    DEF_FUNC(PhysicalAsset,omegaBtoP)
    DEF_FUNC(PhysicalAsset,omegaPtoB)
    DEF_FUNC(PhysicalAsset,posI)
    DEF_FUNC(PhysicalAsset,posP)
    DEF_FUNC(PhysicalAsset,velI)
    DEF_FUNC(PhysicalAsset,velP)
    DEF_FUNC(PhysicalAsset,qI)
    DEF_FUNC(PhysicalAsset,qP)
    DEF_FUNC(PhysicalAsset,omegaI)
    DEF_FUNC(PhysicalAsset,omegaP)
    DEF_FUNC(PhysicalAsset,getAccessor)
    DEF_READWRITE(PhysicalAsset,manager)
    DEF_READWRITE(PhysicalAsset,parent)
    DEF_READWRITE(PhysicalAsset,fullName)
    DEF_READWRITE(PhysicalAsset,team)
    DEF_READWRITE(PhysicalAsset,group)
    DEF_READWRITE(PhysicalAsset,name)
    DEF_READWRITE(PhysicalAsset,readiness)
    DEF_READWRITE(PhysicalAsset,isAlive_)
    DEF_READWRITE(PhysicalAsset,isBoundToParent)
    DEF_READWRITE(PhysicalAsset,motion)
    DEF_READWRITE(PhysicalAsset,pos_prev)
    DEF_READWRITE(PhysicalAsset,vel_prev)
    DEF_READWRITE(PhysicalAsset,agent)
    DEF_READWRITE(PhysicalAsset,children)
    DEF_READWRITE(PhysicalAsset,controllers)
    ;
    EXPOSE_CLASS_WITHOUT_INIT(PhysicalAssetAccessor)
    .def(py::init<std::shared_ptr<PhysicalAsset>>())
    ;
}