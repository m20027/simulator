#include "Ruler.h"
#include "Utility.h"
#include "SimulationManager.h"
#include "Asset.h"
#include "Agent.h"
using namespace util;

Ruler::Ruler(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Callback(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    maxTime=getValueFromJsonKRD(modelConfig,"maxTime",randomGen,1200);
    winner="";
    dones["__all__"]=false;
    endReason=EndReason::NOTYET;
    observables={{"endReason",enumToJson(endReason)}};
}
Ruler::~Ruler(){}
void Ruler::onEpisodeBegin(){
    teams.clear();
    score.clear();
    stepScore.clear();
    nl::json j_teams;
    try{
        j_teams=modelConfig.at("teams");
    }catch(...){
        j_teams="All";
    }
    if(j_teams.is_null()){
        j_teams="All";
    }
    if(j_teams.is_string()){
        std::string team=j_teams;
        if(team=="All"){
            for(auto &&t:manager->getTeams()){
                teams.push_back(t);
                score[t]=0.0;
                stepScore[t]=0.0;
            }
        }else{
            teams.push_back(team);
            score[team]=0.0;
            stepScore[team]=0.0;
        }
    }else if(j_teams.is_array()){
        teams=j_teams.get<std::vector<std::string>>();
        for(auto &&t:teams){
            score[t]=0.0;
            stepScore[t]=0.0;
        }
    }else{
        std::cout<<"modelConfig['teams']="<<j_teams<<std::endl;
        throw std::runtime_error("Invalid designation of 'teams' for Ruler.");
    }
    dones.clear();
    for(auto &&e:manager->getAgents()){
        auto a=getShared(e);
        dones[a->getName()]=!a->isAlive();
    }
    dones["__all__"]=false;
    endReason=EndReason::NOTYET;
    observables["endReason"]=enumToJson(endReason);
}
void Ruler::onStepBegin(){
    for(auto &&t:teams){
        stepScore[t]=0;
    }
}
void Ruler::onInnerStepBegin(){
}
void Ruler::onInnerStepEnd(){
}
void Ruler::onStepEnd(){
    for(auto &&t:teams){
        score[t]+=stepScore[t];
    }
    checkDone();
}
void Ruler::onEpisodeEnd(){
}
void Ruler::checkDone(){
    dones.clear();
    for(auto &&e:manager->getAgents()){
        auto a=getShared(e);
        dones[a->getName()]=!a->isAlive();
    }
    if(manager->getTime()>=maxTime){
        int index=0;
        double maxScore=0;
        for(auto &&t:teams){
            if(index==0 || maxScore<score[t]){
                maxScore=score[t];
                winner=t;
            }
            index++;
        }
        for(auto &e:dones){
            e.second=true;
        }
        dones["__all__"]=true;
        endReason=EndReason::TIMEUP;
        return;
    }
    if(dones.size()==0){
        dones["__all__"]=false;
        return;
    }
    dones["__all__"]=true;
    for(auto &&e:dones){
        if(!e.second){
            dones["__all__"]=false;
        }
    }
    if(dones["__all__"]){
        endReason=EndReason::NO_ONE_ALIVE;
    }
    observables["endReason"]=enumToJson(endReason);
}
double Ruler::getStepScore(const std::string &key){
    if(stepScore.count(key)>0){
        return stepScore[key];
    }else{
        return 0.0;
    }
}
double Ruler::getScore(const std::string &key){
    if(score.count(key)>0){
        return score[key];
    }else{
        return 0.0;
    }
}
double Ruler::getStepScore(const std::shared_ptr<Agent> key){
    return getStepScore(key->getTeam());
}
double Ruler::getScore(const std::shared_ptr<Agent> key){
    return getScore(key->getTeam());
}
nl::json RulerAccessor::dummy=nl::json();
RulerAccessor::RulerAccessor(std::shared_ptr<Ruler> r)
:ruler(r),observables(r ? r->observables : dummy){
}
RulerAccessor::~RulerAccessor(){
}
std::string RulerAccessor::getFactoryBaseName() const{
    return ruler.lock()->getFactoryBaseName();
}
std::string RulerAccessor::getFactoryClassName() const{
    return ruler.lock()->getFactoryClassName();
}
std::string RulerAccessor::getFactoryModelName() const{
    return ruler.lock()->getFactoryModelName();
}
double RulerAccessor::getStepScore(const std::string &key){
    return ruler.lock()->getStepScore(key);
}
double RulerAccessor::getScore(const std::string &key){
    return ruler.lock()->getScore(key);
}
double RulerAccessor::getStepScore(const std::shared_ptr<Agent> key){
    return ruler.lock()->getStepScore(key);
}
double RulerAccessor::getScore(const std::shared_ptr<Agent> key){
    return ruler.lock()->getScore(key);
}
bool RulerAccessor::isinstancePY(py::object cls){
    return py::isinstance(py::cast(ruler),cls);
}

std::shared_ptr<RulerAccessor> Ruler::getAccessor(){
    if(!accessor){
        accessor=std::make_shared<RulerAccessor>(getShared<Ruler>(this->shared_from_this()));
    }
    return accessor;
}

void exportRuler(py::module &m)
{
    using namespace pybind11::literals;
    auto cls=EXPOSE_CLASS(Ruler);
    cls
    DEF_FUNC(Ruler,onEpisodeBegin)
    DEF_FUNC(Ruler,onStepBegin)
    DEF_FUNC(Ruler,onInnerStepBegin)
    DEF_FUNC(Ruler,onInnerStepEnd)
    DEF_FUNC(Ruler,onStepEnd)
    DEF_FUNC(Ruler,onEpisodeEnd)
    DEF_FUNC(Ruler,checkDone)
    .def("getStepScore",py::overload_cast<const std::string&>(&Ruler::getStepScore))
    .def("getStepScore",py::overload_cast<const std::shared_ptr<Agent>>(&Ruler::getStepScore))
    .def("getScore",py::overload_cast<const std::string&>(&Ruler::getScore))
    .def("getScore",py::overload_cast<const std::shared_ptr<Agent>>(&Ruler::getScore))
    DEF_READWRITE(Ruler,dones)
    DEF_READWRITE(Ruler,maxTime)
    DEF_READWRITE(Ruler,teams)
    DEF_READWRITE(Ruler,winner)
    DEF_READWRITE(Ruler,score)
    DEF_READWRITE(Ruler,stepScore)
    DEF_READWRITE(Ruler,observables)
    DEF_READWRITE(Ruler,endReason)
    ;
    py::enum_<Ruler::EndReason>(cls,"EndReason")
    .value("NOTYET",Ruler::EndReason::NOTYET)
    .value("TIMEUP",Ruler::EndReason::TIMEUP)
    .value("NO_ONE_ALIVE",Ruler::EndReason::NO_ONE_ALIVE)
    ;
    EXPOSE_BASE_CLASS_WITHOUT_INIT(RulerAccessor)
    .def(py::init<std::shared_ptr<Ruler>>())
    DEF_FUNC(RulerAccessor,getFactoryBaseName)
    DEF_FUNC(RulerAccessor,getFactoryClassName)
    DEF_FUNC(RulerAccessor,getFactoryModelName)
    .def("getStepScore",py::overload_cast<const std::string&>(&RulerAccessor::getStepScore))
    .def("getStepScore",py::overload_cast<const std::shared_ptr<Agent>>(&RulerAccessor::getStepScore))
    .def("getScore",py::overload_cast<const std::string&>(&RulerAccessor::getScore))
    .def("getScore",py::overload_cast<const std::shared_ptr<Agent>>(&RulerAccessor::getScore))
    .def_property_readonly("observables",[](const RulerAccessor& v){return v.observables;})
    .def("isinstance",py::overload_cast<py::object>(&RulerAccessor::isinstancePY))
    ;
}