#include "MassPointFighter.h"
#include "Utility.h"
#include "Units.h"
#include "CommunicationBuffer.h"
#include "SimulationManager.h"
using namespace util;
MassPointFighter::MassPointFighter(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Fighter(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    //modelConfigで指定するもの
    vMin=getValueFromJsonKRD(modelConfig.at("dynamics"),"vMin",randomGen,150.0);
    vMax=getValueFromJsonKRD(modelConfig.at("dynamics"),"vMax",randomGen,450.0);
    aMin=getValueFromJsonKRD(modelConfig.at("propulsion"),"aMin",randomGen,-gravity);
    aMax=getValueFromJsonKRD(modelConfig.at("propulsion"),"aMax",randomGen,gravity);
    rollMax=deg2rad(getValueFromJsonKRD(modelConfig.at("dynamics"),"rollMax",randomGen,180.0));
    pitchMax=deg2rad(getValueFromJsonKRD(modelConfig.at("dynamics"),"pitchMax",randomGen,30.0));
    yawMax=deg2rad(getValueFromJsonKRD(modelConfig.at("dynamics"),"yawMax",randomGen,30.0));
    observables["spec"].merge_patch({
        {"dynamics",{
            {"vMin",vMin},
            {"vMax",vMax},
            {"aMin",aMin},
            {"aMax",aMax},
            {"rollMax",rollMax},
            {"pitchMax",pitchMax},
            {"yawMax",yawMax}
        }}
    });
}
MassPointFighter::~MassPointFighter(){
}
void MassPointFighter::calcMotion(double dt){
    vel_prev=motion.vel;
    pos_prev=motion.pos;
    double V=motion.vel.norm();
    nl::json ctrl=controllers["FlightController"].lock()->commands["motion"];
    Eigen::Vector3d omegaB=ctrl.at("omegaB");
    double throttle=ctrl.at("throttle");
    motion.omega=relBtoI(omegaB);
    double theta=motion.omega.norm()*dt;
    if(theta>1e-6){
        Eigen::Vector3d ax=motion.omega.normalized();
        Quaternion dq=Quaternion::fromAngle(ax,theta);
        motion.q=dq*motion.q;
    }else{
        Eigen::VectorXd dq=motion.q.dqdwi()*motion.omega*dt;
        motion.q=(motion.q+Quaternion(dq)).normalized();
    }
    double Vaft=std::max(vMin,std::min(vMax,V+throttle*dt));
    motion.vel=relBtoI(Eigen::Vector3d(Vaft,0,0));
    motion.pos+=motion.vel*dt;
    motion.time=manager->getTime()+dt;
    motion.calcQh();
}
MassPointFighter::FlightController::FlightController(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Fighter::FlightController(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
MassPointFighter::FlightController::~FlightController(){
}

nl::json MassPointFighter::FlightController::getDefaultCommand(){
    return {
        {"roll",0.0},
        {"pitch",0.0},
        {"yaw",0.0},
        {"accel",0.0}
    };
}
nl::json MassPointFighter::FlightController::calc(const nl::json &cmd){
    auto p=getShared<MassPointFighter>(parent);
    auto roll=std::clamp(cmd.at("roll").get<double>(),-1.,1.)*p->rollMax;
    auto pitch=std::clamp(cmd.at("pitch").get<double>(),-1.,1.)*p->pitchMax;
    auto yaw=std::clamp(cmd.at("yaw").get<double>(),-1.,1.)*p->yawMax;
    double throttle;
    if(cmd.contains("throttle")){//0〜1
        throttle=p->aMin+(p->aMax-p->aMin)*std::clamp(cmd.at("throttle").get<double>(),0.,1.);
    }else if(cmd.contains("accel")){//-1〜+1
       throttle=p->aMin+(p->aMax-p->aMin)*(1+std::clamp(cmd.at("accel").get<double>(),-1.,1.))*0.5;
    }else{
        throw std::runtime_error("Either 'throttle' or 'accel' must be in cmd keys.");
    }
    return {
        {"omegaB",Eigen::Vector3d(roll,pitch,yaw)},
        {"throttle",throttle}
    };
}

void exportMassPointFighter(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(MassPointFighter)
    DEF_FUNC(MassPointFighter,calcMotion)
    DEF_READWRITE(MassPointFighter,vMin)
    DEF_READWRITE(MassPointFighter,vMax)
    DEF_READWRITE(MassPointFighter,aMin)
    DEF_READWRITE(MassPointFighter,aMax)
    DEF_READWRITE(MassPointFighter,rollMax)
    DEF_READWRITE(MassPointFighter,pitchMax)
    DEF_READWRITE(MassPointFighter,yawMax)
    ;
    EXPOSE_CLASS(MassPointFighter::FlightController)
    DEF_FUNC(MassPointFighter::FlightController,getDefaultCommand)
    DEF_FUNC(MassPointFighter::FlightController,calc)
    .def("calc",[](MassPointFighter::FlightController& v,const py::object &cmd){
        return v.calc(cmd);
    })
    ;
}
