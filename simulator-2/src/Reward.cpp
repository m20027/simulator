#include "Reward.h"
#include "Utility.h"
#include "SimulationManager.h"
#include "Asset.h"
#include "Agent.h"
#include "Ruler.h"
using namespace util;

Reward::Reward(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Callback(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    try{
        j_target=modelConfig.at("target");
    }catch(...){
        j_target="All";
    }
    if(j_target.is_null()){
        j_target="All";
    }
}
Reward::~Reward(){}
void Reward::onEpisodeBegin(){
    target.clear();
    reward.clear();
    totalReward.clear();
    if(j_target.is_string()){
        std::string s_target=j_target;
        if(s_target=="All"){
            for(auto &&t:manager->getTeams()){
                target.push_back(t);
                reward[t]=0.0;
                totalReward[t]=0.0;
            }
        }else{
            target.push_back(s_target);
            reward[s_target]=0.0;
            totalReward[s_target]=0.0;
        }
    }else if(j_target.is_array()){
        target=j_target.get<std::vector<std::string>>();
        for(auto &&t:target){
            reward[t]=0.0;
            totalReward[t]=0.0;
        }
    }else{
        std::cout<<"modelConfig['target']="<<j_target<<std::endl;
        throw std::runtime_error("Invalid designation of 'target' for Reward.");
    }
}
void Reward::onStepBegin(){
    for(auto &&t:target){
        reward[t]=0;
    }
}
void Reward::onStepEnd(){
    for(auto &&t:target){
        totalReward[t]+=reward[t];
    }
}
double Reward::getReward(const std::string &key){
    if(reward.count(key)>0){
        return reward[key];
    }else{
        return 0.0;
    }
}
double Reward::getTotalReward(const std::string &key){
    if(totalReward.count(key)>0){
        return totalReward[key];
    }else{
        return 0.0;
    }
}
double Reward::getReward(const std::shared_ptr<Agent> key){
    throw std::runtime_error("Please override Reward::getReward(const std::shared_ptr<Agent> key).");
}
double Reward::getTotalReward(const std::shared_ptr<Agent> key){
    throw std::runtime_error("Please override Reward::getTotalReward(const std::shared_ptr<Agent> key).");
}
AgentReward::AgentReward(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Reward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
AgentReward::~AgentReward(){}
void AgentReward::onEpisodeBegin(){
    target.clear();
    reward.clear();
    totalReward.clear();
    if(j_target.is_string()){
        std::string s_target=j_target;
        if(s_target=="All"){
            for(auto &&e:manager->getAgents()){
                std::string agentFullName=getShared(e)->getFullName();
                target.push_back(agentFullName);
                reward[agentFullName]=0.0;
                totalReward[agentFullName]=0.0;
            }
        }else{
            target.push_back(s_target);
            reward[s_target]=0.0;
            totalReward[s_target]=0.0;
        }
    }else if(j_target.is_array()){
        target=j_target.get<std::vector<std::string>>();
        for(auto &&t:target){
            reward[t]=0.0;
            totalReward[t]=0.0;
        }
    }else{
        std::cout<<"modelConfig['target']="<<j_target<<std::endl;
        throw std::runtime_error("Invalid designation of 'target' for AgentReward.");
    }
}
double AgentReward::getReward(const std::string &key){
    return Reward::getReward(key);
}
double AgentReward::getTotalReward(const std::string &key){
    return Reward::getTotalReward(key);
}
double AgentReward::getReward(const std::shared_ptr<Agent> key){
    return Reward::getReward(key->getFullName());
}
double AgentReward::getTotalReward(const std::shared_ptr<Agent> key){
    return Reward::getTotalReward(key->getFullName());
}

TeamReward::TeamReward(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Reward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
TeamReward::~TeamReward(){}
void TeamReward::onEpisodeBegin(){
    target.clear();
    reward.clear();
    totalReward.clear();
    if(j_target.is_string()){
        std::string s_target=j_target;
        if(s_target=="All"){
            for(auto &&t:manager->getTeams()){
                target.push_back(t);
                reward[t]=0.0;
                totalReward[t]=0.0;
            }
        }else{
            target.push_back(s_target);
            reward[s_target]=0.0;
            totalReward[s_target]=0.0;
        }
    }else if(j_target.is_array()){
        target=j_target.get<std::vector<std::string>>();
        for(auto &&t:target){
            reward[t]=0.0;
            totalReward[t]=0.0;
        }
    }else{
        std::cout<<"modelConfig['target']="<<j_target<<std::endl;
        throw std::runtime_error("Invalid designation of 'target' for AgentReward.");
    }
}
double TeamReward::getReward(const std::string &key){
    return Reward::getReward(key);
}
double TeamReward::getTotalReward(const std::string &key){
    return Reward::getTotalReward(key);
}
double TeamReward::getReward(const std::shared_ptr<Agent> key){
    return Reward::getReward(key->getTeam());
}
double TeamReward::getTotalReward(const std::shared_ptr<Agent> key){
    return Reward::getTotalReward(key->getTeam());
}

ScoreReward::ScoreReward(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:TeamReward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
ScoreReward::~ScoreReward(){}
void ScoreReward::onStepEnd(){
    for(auto &&e:reward){
        reward[e.first]=manager->getRuler().lock()->getStepScore(e.first);
        totalReward[e.first]+=reward[e.first];
    }
}

void exportReward(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Reward)
    DEF_FUNC(Reward,onEpisodeBegin)
    DEF_FUNC(Reward,onStepBegin)
    DEF_FUNC(Reward,onStepEnd)
    .def("getReward",py::overload_cast<const std::string&>(&Reward::getReward))
    .def("getReward",py::overload_cast<const std::shared_ptr<Agent>>(&Reward::getReward))
    .def("getTotalReward",py::overload_cast<const std::string&>(&Reward::getTotalReward))
    .def("getTotalReward",py::overload_cast<const std::shared_ptr<Agent>>(&Reward::getTotalReward))
    .def_property("j_target",[](const Reward& v){return v.j_target;},[](Reward& v,const py::object& obj){v.j_target=obj;})
    DEF_READWRITE(Reward,target)
    DEF_READWRITE(Reward,reward)
    DEF_READWRITE(Reward,totalReward)
    ;
    EXPOSE_CLASS(AgentReward)
    DEF_FUNC(AgentReward,onEpisodeBegin)
    .def("getReward",py::overload_cast<const std::string&>(&AgentReward::getReward))
    .def("getReward",py::overload_cast<const std::shared_ptr<Agent>>(&AgentReward::getReward))
    .def("getTotalReward",py::overload_cast<const std::string&>(&AgentReward::getTotalReward))
    .def("getTotalReward",py::overload_cast<const std::shared_ptr<Agent>>(&AgentReward::getTotalReward))
    ;
    EXPOSE_CLASS(TeamReward)
    DEF_FUNC(TeamReward,onEpisodeBegin)
    .def("getReward",py::overload_cast<const std::string&>(&TeamReward::getReward))
    .def("getReward",py::overload_cast<const std::shared_ptr<Agent>>(&TeamReward::getReward))
    .def("getTotalReward",py::overload_cast<const std::string&>(&TeamReward::getTotalReward))
    .def("getTotalReward",py::overload_cast<const std::shared_ptr<Agent>>(&TeamReward::getTotalReward))
    ;
    EXPOSE_CLASS(ScoreReward)
    DEF_FUNC(ScoreReward,onStepEnd)
    ;
}
