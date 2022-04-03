#include "Agent.h"
#include <pybind11/stl.h>
#include "Utility.h"
#include "Factory.h"
#include "SimulationManager.h"
#include "PhysicalAsset.h"
using namespace util;
Agent::Agent(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Asset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    manager=instanceConfig.at("manager");
    name=instanceConfig.at("name");
    type=instanceConfig.at("type");
    model=instanceConfig.at("model");
    policy=instanceConfig.at("policy");
    try{
        parents=instanceConfig.at("parents").get<std::map<std::string,std::shared_ptr<PhysicalAssetAccessor>>>();
    }catch(...){
        //
    }
    commands=nl::json::object();
    observables=nl::json::object();
    for(auto &&e:parents){
        commands[e.second->getFullName()]=nl::json::object();
        observables[e.second->getFullName()]={{"decision",nl::json::object()}};
    }
}
Agent::~Agent(){}
bool Agent::isAlive() const{
    bool ret=false;
    for(auto&& e:parents){
        ret=ret || e.second->isAlive();
    }
    return ret;
}
std::string Agent::getTeam() const{
    return (*parents.begin()).second->getTeam();
}
std::string Agent::getGroup() const{
    return (*parents.begin()).second->getGroup();
}
std::string Agent::getName() const{
    return name;
}
std::string Agent::getFullName() const{
    return name+":"+model+":"+policy;
}
std::string Agent::repr() const{
    if(type=="Learning"){
        return name+":Learning("+model+")";
    }else if(type=="Clone"){
        return name+":Clone("+model+")";
    }else if(type=="External"){
        return name+":"+model+"["+policy+"]";
    }else{
        return name+":"+model;
    }
}
void Agent::validate(){
}
void Agent::setDependency(){
    //In the parent PhysicalAsset's setDependency, agent's dependency can be set as follows:
    //  agent.lock()->dependencyChecker->addDependency("perceive",getShared<Asset>(this->shared_from_this()));
}
py::object Agent::observation_space(){
    py::module_ spaces=py::module_::import("gym.spaces");
    return spaces.attr("Discrete")(1);
}
py::object Agent::makeObs(){
    return py::cast((int)0);
}
py::object Agent::action_space(){
    py::module_ spaces=py::module_::import("gym.spaces");
    return spaces.attr("Discrete")(1);
}
void Agent::deploy(py::object action){
    for(auto &&e:parents){
        observables[e.second->getFullName()]={{"decision",nl::json::object()}};
    }
}
void Agent::perceive(bool inReset){
}
void Agent::control(){
    for(auto &&e:parents){
        commands[e.second->getFullName()]=nl::json::object();
    }
}
void Agent::behave(){
}
py::object Agent::convertActionFromAnother(const nl::json& decision,const nl::json& command){
    return py::cast((int)0);
}

ExpertWrapper::ExpertWrapper(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Agent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    whichOutput=modelConfig.at("whichOutput").get<std::string>();
    imitatorModelName=instanceConfig.at("imitatorModelName").get<std::string>();
    expertModelName=instanceConfig.at("expertModelName").get<std::string>();
    expertPolicyName=instanceConfig.at("expertPolicyName").get<std::string>();
    trajectoryIdentifier=instanceConfig.at("identifier").get<std::string>();
    auto mAcc=manager->copy();
    auto dep=DependencyChecker::create(dependencyChecker);
    nl::json sub={
        {"manager",mAcc},
        {"dependencyChecker",dep},
        {"name",name+":expert"},
        {"seed",randomGen()},
        {"parents",parents},
        {"type","Child"},
        {"model",expertModelName},
        {"policy",expertPolicyName}
    };
    expert=manager->generateUnmanagedChild<Agent>(
        "Agent",
        expertModelName,
        sub
    );
    mAcc=manager->copy();
    dep=DependencyChecker::create(dependencyChecker);
    sub["manager"]=mAcc;
    sub["dependencyChecker"]=dep;
    sub["name"]=name+":imitator";
    sub["seed"]=randomGen();
    sub["model"]=expertModelName;
    sub["policy"]=expertPolicyName;
    imitator=manager->generateUnmanagedChild<Agent>(
        "Agent",
        imitatorModelName,
        sub
    );
    imitatorObs=py::none();
    imitatorAction=py::none();
    isInternal=false;
    hasImitatorDeployed=false;
    observables=expert->observables;
}
ExpertWrapper::~ExpertWrapper(){}

std::string ExpertWrapper::repr() const{
    std::string ret=name+":"+type+"("+imitatorModelName+"<-"+expertModelName;
    if(expertPolicyName!="" && expertPolicyName!="Auto"){
        ret+="["+expertPolicyName+"]";
    }
    return ret;
}
void ExpertWrapper::validate(){
    expert->validate();
    imitator->validate();
    observables=expert->observables;
}
py::object ExpertWrapper::observation_space(){
    imitator->observation_space();
    return expert->observation_space();
}
py::object ExpertWrapper::makeObs(){
    imitatorObs=imitator->makeObs();
    return expert->makeObs();
}
py::object ExpertWrapper::action_space(){
    imitator->action_space();
    return expert->action_space();
}
py::object ExpertWrapper::imitator_observation_space(){
    return imitator->observation_space();
}
py::object ExpertWrapper::imitator_action_space(){
    return imitator->action_space();
}
void ExpertWrapper::deploy(py::object action){
    if(expertPolicyName=="Internal"){
        expert->deploy(py::none());
    }else{
        expert->deploy(action);
    }
    observables=expert->observables;
    hasImitatorDeployed=false;
}
void ExpertWrapper::perceive(bool inReset){
    imitator->perceive(inReset);
    expert->perceive(inReset);
    observables=expert->observables;
}
void ExpertWrapper::control(){
    expert->control();
    observables=expert->observables;
    if(!hasImitatorDeployed){
        nl::json decisions=nl::json::object();
        for(auto&&e:expert->observables.items()){
            decisions[e.key()]=e.value().at("decision");
        }
        imitatorAction=imitator->convertActionFromAnother(decisions,expert->commands);
        imitator->deploy(imitatorAction);
        hasImitatorDeployed=true;
    }
    imitator->control();
    if(whichOutput=="Expert"){
        commands=expert->commands;
    }else{
        commands=imitator->commands;
    }
}
void ExpertWrapper::behave(){
    imitator->behave();
    expert->behave();
    observables=expert->observables;
}

MultiPortCombiner::MultiPortCombiner(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Agent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    std::map<std::string,nl::json> childrenConfigs=modelConfig.at("children");
    std::map<std::string,nl::json> buffer;
    std::string childName,childModel,childPort;
    for(auto &&e:childrenConfigs){
        std::string key=e.first;
        nl::json value=e.second;
        try{
            childName=value.at("name").get<std::string>();
        }catch(...){
            childName=name+":"+key;
        }
        childModel=value.at("model").get<std::string>();
        try{
            childPort=value.at("port").get<std::string>();
        }catch(...){
            childPort="0";
        }
        if(parents.count(key)>0){
            //instanciate only when parents for the child actually exist.
            if(buffer.count(childName)>0){
                buffer[childName]["ports"][childPort]=key;
                buffer[childName]["parents"][childPort]=parents[key];
            }else{
                buffer[childName]={
                    {"ports",{{childPort,key}}},
                    {"model",childModel},
                    {"parents",{{childPort,parents[key]}}}
                };
            }
            ports[key]={
                {"childName",childName},
                {"childPort",childPort}
            };
        }
    }
    nl::json sub;
    for(auto &&e:buffer){
        std::string key=e.first;
        nl::json value=e.second;
        auto mAcc=manager->copy();
        auto dep=DependencyChecker::create(dependencyChecker);
        sub={
            {"manager",mAcc},
            {"dependencyChecker",dep},
            {"name",key},
            {"seed",randomGen()},
            {"parents",buffer[key]["parents"]},
            {"type","Child"},
            {"model",buffer[key]["model"]},
            {"policy",policy}
        };
        children[key]=manager->generateUnmanagedChild<Agent>(
            "Agent",
            buffer[key]["model"],
            sub
        );
    }
}
MultiPortCombiner::~MultiPortCombiner(){}
void MultiPortCombiner::validate(){
    for(auto &&e:children){
        e.second->validate();
    }
}
py::object MultiPortCombiner::observation_space(){
    py::module_ spaces=py::module_::import("gym.spaces");
    return spaces.attr("Discrete")(1);
}
py::object MultiPortCombiner::makeObs(){
    return py::cast((int)0);
}
py::object MultiPortCombiner::action_space(){
    py::module_ spaces=py::module_::import("gym.spaces");
    return spaces.attr("Discrete")(1);
}
std::map<std::string,py::object> MultiPortCombiner::actionSplitter(py::object action){
    std::map<std::string,py::object> tmp;
    for(auto &&e:children){
        tmp[e.first]=py::cast((int)0);
    }
    return tmp;
}
void MultiPortCombiner::deploy(py::object action){
    std::map<std::string,py::object> splitted=actionSplitter(action);
    std::shared_ptr<Agent> child;
    for(auto &&e:children){
        if(e.second->isAlive()){
            e.second->deploy(splitted[e.first]);
        }
    }
    for(auto &&e:ports){
        std::string key=e.first;
        child=children[e.second["childName"]];
        std::string parentName=parents[key]->getFullName();
        commands[parentName]=child->commands[parentName];
        observables[parentName]=child->observables[parentName];
    }
}
void MultiPortCombiner::perceive(bool inReset){
    std::shared_ptr<Agent> child;
    for(auto &&e:children){
        if(e.second->isAlive()){
            e.second->perceive(inReset);
        }
    }
    for(auto &&e:ports){
        std::string key=e.first;
        child=children[e.second["childName"]];
        std::string parentName=parents[key]->getFullName();
        commands[parentName]=child->commands[parentName];
        observables[parentName]=child->observables[parentName];
    }
}
void MultiPortCombiner::control(){
    std::shared_ptr<Agent> child;
    for(auto &&e:children){
        if(e.second->isAlive()){
            e.second->control();
        }
    }
    for(auto &&e:ports){
        std::string key=e.first;
        child=children[e.second["childName"]];
        std::string parentName=parents[key]->getFullName();
        commands[parentName]=child->commands[parentName];
        observables[parentName]=child->observables[parentName];
    }
}

SingleAssetAgent::SingleAssetAgent(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Agent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    port=(*parents.begin()).first;
    parent=(*parents.begin()).second;
    parents.clear();
    parents[port]=parent;
}
SingleAssetAgent::~SingleAssetAgent(){}

void exportAgent(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Agent)
    DEF_FUNC(Agent,isAlive)
    DEF_FUNC(Agent,getTeam)
    DEF_FUNC(Agent,getGroup)
    DEF_FUNC(Agent,getName)
    DEF_FUNC(Agent,getFullName)
    .def("__repr__",&Agent::repr)
    DEF_FUNC(Agent,validate)
    DEF_FUNC(Agent,observation_space)
    DEF_FUNC(Agent,makeObs)
    DEF_FUNC(Agent,action_space)
    DEF_FUNC(Agent,deploy)
    DEF_FUNC(Agent,perceive)
    DEF_FUNC(Agent,control)
    DEF_FUNC(Agent,behave)
    DEF_FUNC(Agent,convertActionFromAnother)
    .def("convertActionFromAnother",[](Agent& v,const py::object& decision,const py::object& command){
        return v.convertActionFromAnother(decision,command);
    })
    DEF_READWRITE(Agent,manager)
    DEF_READWRITE(Agent,name)
    DEF_READWRITE(Agent,parents)
    ;
    EXPOSE_CLASS(ExpertWrapper)
    .def("__repr__",&ExpertWrapper::repr)
    DEF_FUNC(ExpertWrapper,validate)
    DEF_FUNC(ExpertWrapper,observation_space)
    DEF_FUNC(ExpertWrapper,makeObs)
    DEF_FUNC(ExpertWrapper,action_space)
    DEF_FUNC(ExpertWrapper,imitator_observation_space)
    DEF_FUNC(ExpertWrapper,imitator_action_space)
    DEF_FUNC(ExpertWrapper,deploy)
    DEF_FUNC(ExpertWrapper,perceive)
    DEF_FUNC(ExpertWrapper,control)
    DEF_FUNC(ExpertWrapper,behave)
    DEF_READWRITE(ExpertWrapper,whichOutput)
    DEF_READWRITE(ExpertWrapper,expert)
    DEF_READWRITE(ExpertWrapper,imitator)
    DEF_READWRITE(ExpertWrapper,imitatorModelName)
    DEF_READWRITE(ExpertWrapper,expertModelName)
    DEF_READWRITE(ExpertWrapper,expertPolicyName)
    DEF_READWRITE(ExpertWrapper,trajectoryIdentifier)
    DEF_READWRITE(ExpertWrapper,imitatorObs)
    DEF_READWRITE(ExpertWrapper,imitatorAction)
    DEF_READWRITE(ExpertWrapper,isInternal)
    DEF_READWRITE(ExpertWrapper,hasImitatorDeployed)
    ;
    EXPOSE_CLASS(MultiPortCombiner)
    DEF_FUNC(MultiPortCombiner,validate)
    DEF_FUNC(MultiPortCombiner,observation_space)
    DEF_FUNC(MultiPortCombiner,makeObs)
    DEF_FUNC(MultiPortCombiner,action_space)
    DEF_FUNC(MultiPortCombiner,actionSplitter)
    DEF_FUNC(MultiPortCombiner,deploy)
    DEF_FUNC(MultiPortCombiner,perceive)
    DEF_FUNC(MultiPortCombiner,control)
    DEF_READWRITE(MultiPortCombiner,ports)
    DEF_READWRITE(MultiPortCombiner,children)
    ;
    EXPOSE_CLASS(SingleAssetAgent)
    DEF_READWRITE(SingleAssetAgent,parent)
    DEF_READWRITE(SingleAssetAgent,port)
    ;
}

