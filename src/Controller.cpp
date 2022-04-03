#include "Controller.h"
#include <pybind11/stl.h>
#include "Utility.h"
#include "Factory.h"
#include "SimulationManager.h"
#include "Asset.h"
using namespace util;

Controller::Controller(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Asset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
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
    if(parent.expired()){
        throw std::runtime_error("Controller must have a parent. fullName="+fullName);
    }
}
Controller::~Controller(){}
bool Controller::isAlive() const{
    return parent.lock()->isAlive();
}
std::string Controller::getTeam() const{
    return team;
}
std::string Controller::getGroup() const{
    return group;
}
std::string Controller::getName() const{
    return name;
}
std::string Controller::getFullName() const{
    return fullName;
}
void Controller::validate(){
}
void Controller::perceive(bool inReset){
}
void Controller::control(){
}
void Controller::behave(){
}
void Controller::kill(){
    observables=nl::json::object();
    commands=nl::json::object();
}

void exportController(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Controller)
    DEF_FUNC(Controller,isAlive)
    DEF_FUNC(Controller,getTeam)
    DEF_FUNC(Controller,getGroup)
    DEF_FUNC(Controller,getName)
    DEF_FUNC(Controller,getFullName)
    DEF_FUNC(Controller,validate)
    DEF_FUNC(Controller,perceive)
    DEF_FUNC(Controller,control)
    DEF_FUNC(Controller,behave)
    DEF_FUNC(Controller,kill)
    DEF_READWRITE(Controller,manager)
    DEF_READWRITE(Controller,name)
    DEF_READWRITE(Controller,parent)
    ;
}

