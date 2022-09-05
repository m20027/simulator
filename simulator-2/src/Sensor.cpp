#include "Sensor.h"
#include "Utility.h"
#include "Units.h"
#include "SimulationManager.h"
#include "Agent.h"
#include "Fighter.h"
#include "Missile.h"
#include <boost/uuid/nil_generator.hpp>
using namespace util;

Sensor::Sensor(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:PhysicalAsset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
Sensor::~Sensor(){}
void Sensor::setDependency(){
}
Sensor3D::Sensor3D(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Sensor(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
Sensor3D::~Sensor3D(){}
std::pair<bool,std::vector<Track3D>> Sensor3D::isTrackingAny(){
    return std::make_pair(false,std::vector<Track3D>());
}
std::pair<bool,Track3D> Sensor3D::isTracking(std::weak_ptr<PhysicalAsset> target_){
    return std::make_pair(false,Track3D());
}
std::pair<bool,Track3D> Sensor3D::isTracking(const Track3D& target_){
    return std::make_pair(false,Track3D());
}
std::pair<bool,Track3D> Sensor3D::isTracking(const boost::uuids::uuid& target_){
    return std::make_pair(false,Track3D());
}
Sensor2D::Sensor2D(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Sensor(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
Sensor2D::~Sensor2D(){}
std::pair<bool,std::vector<Track2D>> Sensor2D::isTrackingAny(){
    return std::make_pair(false,std::vector<Track2D>());
}
std::pair<bool,Track2D> Sensor2D::isTracking(std::weak_ptr<PhysicalAsset> target_){
    return std::make_pair(false,Track2D());
}
std::pair<bool,Track2D> Sensor2D::isTracking(const Track2D& target_){
    return std::make_pair(false,Track2D());
}
std::pair<bool,Track2D> Sensor2D::isTracking(const boost::uuids::uuid& target_){
    return std::make_pair(false,Track2D());
}

AircraftRadar::AircraftRadar(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Sensor3D(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    Lref=getValueFromJsonKR(modelConfig,"Lref",randomGen);
    thetaFOR=deg2rad(getValueFromJsonKR(modelConfig,"thetaFOR",randomGen));
    observables={
        {"spec",{
            {"Lref",Lref},
            {"thetaFOR",thetaFOR}
        }}
    };
}
AircraftRadar::~AircraftRadar(){}
void AircraftRadar::setDependency(){
    //no dependency
}
void AircraftRadar::perceive(bool inReset){
    //PhysicalAsset::perceive(inReset);//Sensors don't need motion and isAlive as observable.
    track.clear();
    for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
        return asset->getTeam()!=getTeam() && isinstance<Fighter>(asset);
    })){
        auto f=getShared<Fighter>(e);
        if(f->isAlive()){
            Eigen::Vector3d fpos=f->posI();
            Eigen::Vector3d rpos=absItoB(fpos);
            double L=rpos.norm();
            if(rpos(0)>L*cos(thetaFOR) && L<=Lref*pow(f->rcsScale,0.25)){
                Eigen::Vector3d fvel=f->velI();
                track.push_back(Track3D(f,fpos,fvel,manager->getTime()));
            }
        }
    }
    observables["track"]=track;
}
std::pair<bool,std::vector<Track3D>> AircraftRadar::isTrackingAny(){
    if(track.size()>0){
        std::vector<Track3D> ret;
        for(auto& e:track){
            ret.push_back(e);
        }
        return std::make_pair(true,std::move(ret));
    }else{
        return std::make_pair(false,std::vector<Track3D>());
    }
}
std::pair<bool,Track3D> AircraftRadar::isTracking(std::weak_ptr<PhysicalAsset> target_){
    if(target_.expired()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.lock()->uuid);
    }
}
std::pair<bool,Track3D> AircraftRadar::isTracking(const Track3D& target_){
    if(target_.is_none()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.truth);
    }
}
std::pair<bool,Track3D> AircraftRadar::isTracking(const boost::uuids::uuid& target_){
    if(target_==boost::uuids::nil_uuid()){
        return std::make_pair(false,Track3D());
    }
    for(auto& e:track){
        if(e.isSame(target_)){
            return std::make_pair(true,e);
        }
    }
    return std::make_pair(false,Track3D());
}
MWS::MWS(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Sensor2D(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    observables={
        {"spec",nl::json::object()}
    };
}
MWS::~MWS(){}
void MWS::setDependency(){
    for(auto&& asset:manager->getAssets()){
        if(asset.lock()->getTeam()!=team && isinstance<Missile>(asset)){
            auto m=getShared<const Missile>(asset);
            dependencyChecker->addDependency(SimPhase::PERCEIVE,m->sensor.lock());
        }
    }
}
void MWS::perceive(bool inReset){
    //PhysicalAsset::perceive(inReset);//Sensors don't need motion and isAlive as observable.
    track.clear();
    for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
        return asset->getTeam()!=getTeam() && isinstance<Missile>(asset);
    })){
        auto m=getShared<Missile>(e);
        if(m->hasLaunched && m->isAlive() && m->sensor.lock()->isActive){
            std::pair<bool,Track3D> r=m->sensor.lock()->isTracking(Track3D(parent));
            if(r.first){
                track.push_back(Track2D(m,parent.lock()->posI()));//真値を入れる
            }
        }
    }
    observables["track"]=track;
}
std::pair<bool,std::vector<Track2D>> MWS::isTrackingAny(){
    if(track.size()>0){
       std::vector<Track2D> ret;
       for(auto& e:track){
           ret.push_back(e);
       }
        return std::make_pair(true,std::move(ret));
    }else{
        return std::make_pair(false,std::vector<Track2D>());
    }
}
std::pair<bool,Track2D> MWS::isTracking(std::weak_ptr<PhysicalAsset> target_){
    if(target_.expired()){
        return std::make_pair(false,Track2D());
    }else{
        return isTracking(target_.lock()->uuid);
    }
}
std::pair<bool,Track2D> MWS::isTracking(const Track2D& target_){
    if(target_.is_none()){
        return std::make_pair(false,Track2D());
    }else{
        return isTracking(target_.truth);
    }
}
std::pair<bool,Track2D> MWS::isTracking(const boost::uuids::uuid& target_){
    if(target_==boost::uuids::nil_uuid()){
        return std::make_pair(false,Track2D());
    }
    for(auto& e:track){
        if(e.isSame(target_)){
            return std::make_pair(true,e);
        }
    }
    return std::make_pair(false,Track2D());
}

MissileSensor::MissileSensor(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Sensor3D(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    Lref=getValueFromJsonKR(modelConfig,"Lref",randomGen);
    thetaFOR=deg2rad(getValueFromJsonKR(modelConfig,"thetaFOR",randomGen));
    thetaFOV=deg2rad(getValueFromJsonKR(modelConfig,"thetaFOV",randomGen));
    isActive=false;
    estTPos<<0,0,0;
    observables={
        {"spec",{
            {"Lref",Lref},
            {"thetaFOR",thetaFOR},
            {"thetaFOV",thetaFOV}
        }}
    };
}
MissileSensor::~MissileSensor(){}
void MissileSensor::setDependency(){
    dependencyChecker->addDependency(SimPhase::CONTROL,parent.lock());
}
void MissileSensor::perceive(bool inReset){
    //PhysicalAsset::perceive(inReset);//Sensors don't need motion and isAlive as observable.
    track.clear();
    if(isActive){
        Eigen::Vector3d steer=absItoB(estTPos).normalized();
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->getTeam()!=getTeam() && isinstance<Fighter>(asset);
        })){
            auto f=getShared<Fighter>(e);
            if(f->isAlive()){
                Eigen::Vector3d fpos=f->posI();
                Eigen::Vector3d rpos=absItoB(fpos);
                double L=rpos.norm();
                if(rpos(0)>L*cos(thetaFOR) && L<=Lref && rpos.dot(steer)>L*cos(thetaFOV)){
                    Eigen::Vector3d fvel=f->velI();
                    track.push_back(Track3D(f,fpos,fvel,manager->getTime()));
                }
            }
        }
    }
    observables["track"]=track;
}
void MissileSensor::control(){
    nl::json cmd=parent.lock()->commands["Sensor"];
    for(auto&& elem:cmd){
        std::string cmdName=elem.at("name");
        if(cmdName=="steering"){
            Eigen::Vector3d p=elem.at("estTPos");
            Eigen::Vector3d v=elem.at("estTVel");
            estTPos=p+v*manager->getBaseTimeStep();
        }else if(cmdName=="activate"){
            if(!isActive){
                target=elem.at("target");
                isActive=true;
            }
        }
    }
}
void MissileSensor::kill(){
    target=Track3D();
    isActive=false;
    this->PhysicalAsset::kill();
}
std::pair<bool,std::vector<Track3D>> MissileSensor::isTrackingAny(){
    if(track.size()>0){
        std::vector<Track3D> ret;
        for(auto& e:track){
            ret.push_back(e);
        }
        return std::make_pair(true,std::move(ret));
    }else{
        return std::make_pair(false,std::vector<Track3D>());
    }
}
std::pair<bool,Track3D> MissileSensor::isTracking(std::weak_ptr<PhysicalAsset> target_){
    if(target_.expired()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.lock()->uuid);
    }
}
std::pair<bool,Track3D> MissileSensor::isTracking(const Track3D& target_){
    if(target_.is_none()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.truth);
    }
}
std::pair<bool,Track3D> MissileSensor::isTracking(const boost::uuids::uuid& target_){
    if(target_==boost::uuids::nil_uuid()){
        return std::make_pair(false,Track3D());
    }
    if(isActive){
        for(auto& e:track){
           if(e.isSame(target_)){
               return std::make_pair(true,e);
            }
        }
    }
    return std::make_pair(false,Track3D());
}

void exportSensor(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Sensor)
    DEF_FUNC(Sensor,setDependency)
    ;
    EXPOSE_CLASS(Sensor3D)
    DEF_FUNC(Sensor3D,isTrackingAny)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&Sensor3D::isTracking))
    .def("isTracking",py::overload_cast<const Track3D&>(&Sensor3D::isTracking))
    .def("isTracking",py::overload_cast<const boost::uuids::uuid&>(&Sensor3D::isTracking))
    ;
    EXPOSE_CLASS(Sensor2D)
    DEF_FUNC(Sensor2D,isTrackingAny)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&Sensor2D::isTracking))
    .def("isTracking",py::overload_cast<const Track2D&>(&Sensor2D::isTracking))
    .def("isTracking",py::overload_cast<const boost::uuids::uuid&>(&Sensor2D::isTracking))
    ;
    EXPOSE_CLASS(AircraftRadar)
    DEF_FUNC(AircraftRadar,setDependency)
    DEF_FUNC(AircraftRadar,perceive)
    DEF_FUNC(AircraftRadar,isTrackingAny)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&AircraftRadar::isTracking))
    .def("isTracking",py::overload_cast<const Track3D&>(&AircraftRadar::isTracking))
    .def("isTracking",py::overload_cast<const boost::uuids::uuid&>(&AircraftRadar::isTracking))
    DEF_READWRITE(AircraftRadar,Lref)
    DEF_READWRITE(AircraftRadar,thetaFOR)
    DEF_READWRITE(AircraftRadar,track)
    ;
    EXPOSE_CLASS(MWS)
    DEF_FUNC(MWS,setDependency)
    DEF_FUNC(MWS,perceive)
    DEF_FUNC(MWS,isTrackingAny)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&MWS::isTracking))
    .def("isTracking",py::overload_cast<const Track2D&>(&MWS::isTracking))
    .def("isTracking",py::overload_cast<const boost::uuids::uuid&>(&MWS::isTracking))
    DEF_READWRITE(MWS,track)
    ;
    EXPOSE_CLASS(MissileSensor)
    DEF_FUNC(MissileSensor,setDependency)
    DEF_FUNC(MissileSensor,perceive)
    DEF_FUNC(MissileSensor,control)
    DEF_FUNC(MissileSensor,kill)
    DEF_FUNC(MissileSensor,isTrackingAny)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&MissileSensor::isTracking))
    .def("isTracking",py::overload_cast<const Track3D&>(&MissileSensor::isTracking))
    .def("isTracking",py::overload_cast<const boost::uuids::uuid&>(&MissileSensor::isTracking))
    DEF_READWRITE(MissileSensor,Lref)
    DEF_READWRITE(MissileSensor,thetaFOR)
    DEF_READWRITE(MissileSensor,thetaFOV)
    DEF_READWRITE(MissileSensor,isActive)
    DEF_READWRITE(MissileSensor,estTPos)
    DEF_READWRITE(MissileSensor,track)
    DEF_READWRITE(MissileSensor,target)
    ;
}