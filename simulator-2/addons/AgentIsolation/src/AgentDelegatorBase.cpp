#include "AgentDelegatorBase.h"
#include <algorithm>
#include <iomanip>
#include <pybind11/numpy.h>
#include <pybind11/eigen.h>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/MathUtility.h>
#include <ASRCAISim1/Units.h>
#include <ASRCAISim1/MassPointFighter.h>
#include <ASRCAISim1/CoordinatedFighter.h>
#include <ASRCAISim1/Missile.h>
#include <ASRCAISim1/Track.h>
#include <ASRCAISim1/SimulationManager.h>
#include <ASRCAISim1/Ruler.h>
using namespace util;
AgentDelegatorBase::AgentDelegatorBase(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Agent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    for(auto&& e:parents){
        parentFullNameToPort[e.second->getFullName()]=e.first;
    }
    data=nl::json::object();
}
AgentDelegatorBase::~AgentDelegatorBase(){}

nl::json AgentDelegatorBase::makeSimulationState(){
    auto teams=manager->getTeams();
    data["manager"]={
        {"agentInterval",manager->getAgentInterval()}, //1stepあたりのtick数
        {"baseTimeStep",manager->getBaseTimeStep()}, //1tickの時間間隔[s]
        {"teams",teams}, //場に存在する陣営名のリスト。原則として"Blue"と"Red"の二つ。
        {"tick",manager->getTickCount()}, //現在のtick数
        {"time",manager->getTime()}, //現在時刻[s]
    };
    auto ruler=manager->getRuler();
    data["ruler"]=ruler->observables;
    data["ruler"]["stepScore"]=nl::json::object();
    data["ruler"]["score"]=nl::json::object();
    data["ruler"]["factoryBaseName"]=ruler->getFactoryBaseName();
    data["ruler"]["factoryClassName"]=ruler->getFactoryClassName();
    data["ruler"]["factoryModelName"]=ruler->getFactoryModelName();
    nl::json& stepScore=data["ruler"]["stepScore"];
    nl::json& score=data["ruler"]["score"];
    for(auto&& team:teams){
        stepScore[team]=ruler->getStepScore(team);
        score[team]=ruler->getScore(team);
    }
    data["agent"]={
        {"name",name},
        {"model",model},
        {"policy",policy},
        {"seed",instanceConfig.at("seed")}
    };
    data["agent"]["parents"]=nl::json::object();
    nl::json& parentsInfo=data["agent"]["parents"];
    data["parents"]=nl::json::object();
    nl::json& parentsObs=data["parents"];
    for(auto&& e:parents){
        parentsInfo[e.first]={
            {"fullName",e.second->getFullName()},
            {"factoryBaseName",e.second->getFactoryBaseName()},
            {"factoryClassName",e.second->getFactoryClassName()},
            {"factoryModelName",e.second->getFactoryModelName()}
        };
        parentsObs[e.second->getFullName()]=e.second->observables;
    }
    return data;
}
void AgentDelegatorBase::parseAgentState(const nl::json& data_){
    if(data_.contains("observables")){
        observables=data_.at("observables");
    }
    if(data_.contains("commands")){
        commands=data_.at("commands");
        for(auto& e:commands.items()){
            if(e.value().contains("flightControllerMode")){
                auto mode=e.value().at("flightControllerMode").get<std::string>();
                auto f=getShared<FighterAccessor>(parents[parentFullNameToPort[e.key()]]);
                f->setFlightControllerMode(mode);
                e.value().erase("flightControllerMode");
            }
        }
    }
}

void exportAgentDelegatorBase(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(AgentDelegatorBase)
    DEF_FUNC(AgentDelegatorBase,makeSimulationState)
    DEF_FUNC(AgentDelegatorBase,parseAgentState)
    ;
}

