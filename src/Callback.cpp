#include "Callback.h"
#include "Utility.h"
#include "SimulationManager.h"
using namespace util;

Callback::Callback(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Entity(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    manager=instanceConfig.at("manager");
    name=instanceConfig.at("name");
    try{
        acceptReconfigure=modelConfig.at("acceptReconfigure");
    }catch(...){
        acceptReconfigure=false;
    }
    firstTick[SimPhase::ON_EPISODE_BEGIN]=0;//meaningless but defined for same interface
    firstTick[SimPhase::ON_STEP_BEGIN]=0;
    if(instanceConfig.contains("firstTick")){
        firstTick[SimPhase::ON_INNERSTEP_BEGIN]=getValueFromJsonKRD(instanceConfig,"firstTick",randomGen,0);
    }else{
        firstTick[SimPhase::ON_INNERSTEP_BEGIN]=getValueFromJsonKRD(modelConfig,"firstTick",randomGen,0);
    }
    firstTick[SimPhase::ON_INNERSTEP_END]=firstTick[SimPhase::ON_INNERSTEP_BEGIN]+1;
    firstTick[SimPhase::ON_STEP_END]=manager->getAgentInterval();
    firstTick[SimPhase::ON_EPISODE_END]=0;//meaningless but defined for same interface
    interval[SimPhase::ON_EPISODE_BEGIN]=std::numeric_limits<std::uint64_t>::max();//meaningless but defined for same interface
    interval[SimPhase::ON_STEP_BEGIN]=manager->getAgentInterval();
    if(instanceConfig.contains("interval")){
        interval[SimPhase::ON_INNERSTEP_BEGIN]=getValueFromJsonKRD(instanceConfig,"interval",randomGen,1);
    }else{
        interval[SimPhase::ON_INNERSTEP_BEGIN]=getValueFromJsonKRD(modelConfig,"interval",randomGen,1);
    }
    interval[SimPhase::ON_INNERSTEP_END]=interval[SimPhase::ON_INNERSTEP_BEGIN];
    interval[SimPhase::ON_STEP_END]=manager->getAgentInterval();
    interval[SimPhase::ON_EPISODE_END]=std::numeric_limits<std::uint64_t>::max();//meaningless but defined for same interface
}
Callback::~Callback(){}
std::string Callback::getName() const{
    return name;
}
void Callback::onEpisodeBegin(){
}
void Callback::onStepBegin(){
}
void Callback::onInnerStepBegin(){
}
void Callback::onInnerStepEnd(){
}
void Callback::onStepEnd(){
}
void Callback::onEpisodeEnd(){
}

void exportCallback(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Callback)
    DEF_FUNC(Callback,getName)
    DEF_FUNC(Callback,onEpisodeBegin)
    DEF_FUNC(Callback,onStepBegin)
    DEF_FUNC(Callback,onInnerStepBegin)
    DEF_FUNC(Callback,onInnerStepEnd)
    DEF_FUNC(Callback,onStepEnd)
    DEF_FUNC(Callback,onEpisodeEnd)
    DEF_READWRITE(Callback,manager)
    DEF_READWRITE(Callback,name)
    DEF_READWRITE(Callback,acceptReconfigure)
    ;
}