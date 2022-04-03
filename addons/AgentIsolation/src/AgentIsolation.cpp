#include "AgentIsolation.h"
#include <ASRCAISim1/CoordinatedFighter.h>
using namespace util;

RulerForIsolation::RulerForIsolation(const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_)
:RulerForIsolation(
    Ruler::create<Ruler>(nl::json(nullptr),nl::json(nullptr)),
    factoryBaseName_,factoryClassName_,factoryModelName_){
}
RulerForIsolation::RulerForIsolation(std::shared_ptr<Ruler> r,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_)
:RulerAccessor(r),dummyRuler(r){
    dummyRuler->factoryBaseName=factoryBaseName_;
    dummyRuler->factoryClassName=factoryClassName_;
    dummyRuler->factoryModelName=factoryModelName_;
}
RulerForIsolation::~RulerForIsolation(){
}
void RulerForIsolation::setData(const nl::json& data_){
    dummyRuler->observables=data_;
    dummyRuler->score=data_.at("score").get<std::map<std::string,double>>();
    dummyRuler->stepScore=data_.at("stepScore").get<std::map<std::string,double>>();
    dummyRuler->observables.erase("score");
    dummyRuler->observables.erase("stepScore");
    dummyRuler->observables.erase("factoryBaseName");
    dummyRuler->observables.erase("factoryClassName");
}
bool RulerForIsolation::isinstancePY(py::object cls){
    return Factory::issubclassof(cls,dummyRuler->getFactoryBaseName(),dummyRuler->getFactoryClassName());
}
FighterForIsolation::FighterForIsolation(const Factory& factory,const std::string& fullName_,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_)
:FighterForIsolation(
    CoordinatedFighter::create<CoordinatedFighter>(nl::json(nullptr),nl::json(nullptr)),
    factory,fullName_,factoryBaseName_,factoryClassName_,factoryModelName_){
}
FighterForIsolation::FighterForIsolation(std::shared_ptr<Fighter> a,const Factory& factory,const std::string& fullName_,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_)
:FighterAccessor(a),dummyFighter(a){
    dummyFighter->fullName=fullName_;
    if(fullName_.find("/")>=0){
        dummyFighter->team=fullName_.substr(0,fullName_.find("/"));
        dummyFighter->group=fullName_.substr(0,fullName_.rfind("/"));
        dummyFighter->name=fullName_.substr(fullName_.rfind("/")+1);
    }else{
        dummyFighter->team=dummyFighter->group=dummyFighter->name=fullName_;
    }
    dummyFighter->factoryBaseName=factoryBaseName_;
    dummyFighter->factoryClassName=factoryClassName_;
    dummyFighter->factoryModelName=factoryModelName_;
    hasSetFlightControllerModeRequested=false;
    flightControllerNameCommand="fromDirAndVel";
    //getRmax用にMissileを生成(rangeTableは生成済みのものを指定すること。)
    nl::json factoryConfigs=factory.getModelConfig();
    nl::json fighterConfig=factoryConfigs.at(factoryBaseName_).at(factoryModelName_);
    std::string missileModelName=fighterConfig.at("/config/weapon/missile"_json_pointer).get<std::string>();
    nl::json missileConfig=factoryConfigs.at("PhysicalAsset").at(missileModelName);
    dummyMissile=Missile::create<Missile>(nl::json(nullptr),nl::json(nullptr));
    dummyMissile->parent=dummyFighter;
    dummyMissile->modelConfig=missileConfig.at("config");
    dummyMissile->validate();//rangeTableの読み込み
}
FighterForIsolation::~FighterForIsolation(){
}
bool FighterForIsolation::isAlive() const{
    return dummyFighter->observables.at("isAlive");
}
bool FighterForIsolation::isinstancePY(py::object cls){
    return Factory::issubclassof(cls,dummyFighter->getFactoryBaseName(),dummyFighter->getFactoryClassName());
}
void FighterForIsolation::setFlightControllerMode(const std::string& ctrlName){
    hasSetFlightControllerModeRequested=true;
    flightControllerNameCommand=ctrlName;
}
double FighterForIsolation::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    return dummyMissile->getRmax(rs,vs,rt,vt);
}
double FighterForIsolation::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
    return dummyMissile->getRmax(rs,vs,rt,vt,aa);
}
void FighterForIsolation::setData(const nl::json& data_){
    dummyFighter->observables=data_;
}

DummyDependencyChecker::DummyDependencyChecker()
:DependencyChecker(std::shared_ptr<SimulationManager>(nullptr)){
}
DummyDependencyChecker::~DummyDependencyChecker(){}
void DummyDependencyChecker::clearReadiness(){
    throw std::runtime_error("An isolated agent cannot use DependencyChecker.");
}
bool DummyDependencyChecker::checkReadiness(const SimPhase &phase){
    throw std::runtime_error("An isolated agent cannot use DependencyChecker.");
    return false;
}
void DummyDependencyChecker::clearDependency(){
    throw std::runtime_error("An isolated agent cannot use DependencyChecker.");
}
void DummyDependencyChecker::addDependency(const SimPhase& phase,const std::string& fullName_){
    throw std::runtime_error("An isolated agent cannot use DependencyChecker.");
}
void DummyDependencyChecker::addDependency(const SimPhase& phase,std::shared_ptr<Asset> asset){
    throw std::runtime_error("An isolated agent cannot use DependencyChecker.");
}
std::shared_ptr<DummyDependencyChecker> DummyDependencyChecker::from_json_ref(const nl::json& j){
    return j;
}
std::weak_ptr<DummyDependencyChecker> DummyDependencyChecker::from_json_weakref(const nl::json& j){
    return j;
}
std::shared_ptr<DummyDependencyChecker> DummyDependencyChecker::create(){
    return std::make_shared<DummyDependencyChecker>();
}

SimulationManagerForIsolation::SimulationManagerForIsolation(const nl::json& agentModelConfig_)
:SimulationManagerAccessorForAgent(std::shared_ptr<SimulationManager>(nullptr)){
    agentModelConfig=agentModelConfig_;
    ruler=std::shared_ptr<RulerForIsolation>(nullptr);
    factory.addModelsFromJson(agentModelConfig.at("Factory"));
}
SimulationManagerForIsolation::~SimulationManagerForIsolation(){}
std::shared_ptr<SimulationManagerForIsolation> SimulationManagerForIsolation::create(const nl::json& agentModelConfig_){
    return std::make_shared<SimulationManagerForIsolation>(agentModelConfig_);
}
void SimulationManagerForIsolation::setSimulationState(const nl::json& data_){
    data=data_.at("manager");
    if(!ruler){
        std::string factoryBaseName=data_.at("ruler").at("factoryBaseName");
        std::string factoryClassName=data_.at("ruler").at("factoryClassName");
        std::string factoryModelName=data_.at("ruler").at("factoryModelName");
        ruler=std::make_shared<RulerForIsolation>(factoryBaseName,factoryClassName,factoryModelName);
    }
    ruler->setData(data_.at("ruler"));
    const nl::json& parentsData=data_.at("parents");
    for(auto&& e:parentsData.items()){
        if(parents.count(e.key())>0){
            auto f=parents[e.key()];
            f->setData(e.value());
        }
    }
}
nl::json SimulationManagerForIsolation::makeAgentState(const std::string& agentFullName){
    auto agent=agents[agentFullName];
    nl::json ret={
        {"observables",agent->observables},
        {"commands",agent->commands}
    };
    for(auto&& e:agent->parents){
        auto f=getShared<FighterForIsolation>(e.second);
        if(f->hasSetFlightControllerModeRequested){
            ret["commands"].at(f->getFullName())["flightControllerMode"]=f->flightControllerNameCommand;
            f->hasSetFlightControllerModeRequested=false;
        }
    }
    return ret;
}
void SimulationManagerForIsolation::clear(){
    data=nl::json::object();
    agents.clear();
    ruler=std::shared_ptr<RulerForIsolation>(nullptr);
    parents.clear();
}
nl::json SimulationManagerForIsolation::initialize(const std::string& agentFullName,const nl::json& data_){
    const nl::json& agentInfo=data_.at("agent");
    nl::json instanceConfig=nl::json::object();
    auto dep=std::make_shared<DummyDependencyChecker>();
    instanceConfig["manager"]=this->weak_from_this();
    instanceConfig["dependencyChecker"]=dep;
    instanceConfig["name"]=agentInfo.at("name");
    instanceConfig["type"]="External";
    instanceConfig["model"]=agentInfo.at("model");
    instanceConfig["policy"]=agentInfo.at("policy");
    instanceConfig["seed"]=agentInfo.at("seed");
    instanceConfig["parents"]=nl::json::object();
    const nl::json& parentsInfo=agentInfo.at("parents");
    for(auto&& e:parentsInfo.items()){
        std::string port=e.key();
        std::string fullName=e.value().at("fullName");
        std::string factoryBaseName=e.value().at("factoryBaseName");
        std::string factoryClassName=e.value().at("factoryClassName");
        std::string factoryModelName=e.value().at("factoryModelName");
        parents[fullName]=std::make_shared<FighterForIsolation>(factory,fullName,factoryBaseName,factoryClassName,factoryModelName);
        instanceConfig["parents"][port]=parents[fullName];
    }
    setSimulationState(data_);
    agents[agentFullName]=factory.create<Agent>("Agent",instanceConfig["model"],instanceConfig);
    return makeAgentState(agentFullName);
}
py::tuple SimulationManagerForIsolation::action_space(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    return py::make_tuple(makeAgentState(agentFullName),agent->action_space());
}
py::tuple SimulationManagerForIsolation::observation_space(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    return py::make_tuple(makeAgentState(agentFullName),agent->observation_space());
}
py::tuple SimulationManagerForIsolation::makeObs(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    return py::make_tuple(makeAgentState(agentFullName),agent->makeObs());
}
nl::json SimulationManagerForIsolation::deploy(const std::string& agentFullName,const py::tuple& data_){
    setSimulationState(data_[0].cast<nl::json>());
    auto agent=agents[agentFullName];
    agent->deploy(data_[1]);
    return makeAgentState(agentFullName);
}
nl::json SimulationManagerForIsolation::validate(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    agent->validate();
    return makeAgentState(agentFullName);
}
nl::json SimulationManagerForIsolation::perceive(const std::string& agentFullName,const py::tuple& data_){
    setSimulationState(data_[0].cast<nl::json>());
    auto agent=agents[agentFullName];
    bool inReset=data_[1].cast<bool>();
    agent->perceive(inReset);
    return makeAgentState(agentFullName);
}
nl::json SimulationManagerForIsolation::control(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    agent->control();
    return makeAgentState(agentFullName);
}
nl::json SimulationManagerForIsolation::behave(const std::string& agentFullName,const nl::json& data_){
    setSimulationState(data_);
    auto agent=agents[agentFullName];
    agent->behave();
    return makeAgentState(agentFullName);
}
bool SimulationManagerForIsolation::expired() const noexcept{
    return false;
}
double SimulationManagerForIsolation::getTime() const{
    return data.at("time");
}
double SimulationManagerForIsolation::getBaseTimeStep() const{
    return data.at("baseTimeStep");
}
std::uint64_t SimulationManagerForIsolation::getTickCount() const{
    return data.at("tick");
}
std::uint64_t SimulationManagerForIsolation::getAgentInterval() const{
    return data.at("agentInterval");
}
std::vector<std::string> SimulationManagerForIsolation::getTeams() const{
    return std::move(data.at("teams").get<std::vector<std::string>>());
}
std::shared_ptr<RulerAccessor> SimulationManagerForIsolation::getRuler() const{
    return ruler;
}
std::shared_ptr<SimulationManagerAccessorForAgent> SimulationManagerForIsolation::copy(){
    auto ret=SimulationManagerForIsolation::create(agentModelConfig);
    ret->data=this->data;
    return ret;
}
bool SimulationManagerForIsolation::requestInvitationToCommunicationBuffer(const std::string& bufferName,std::shared_ptr<Asset> asset){
    throw std::runtime_error("An isolated agent cannot request an invitation to a CommunicationBuffer.");
    return false;
}
std::shared_ptr<SimulationManagerForIsolation> SimulationManagerForIsolation::from_json_ref(const nl::json& j){
    return j;
}
std::weak_ptr<SimulationManagerForIsolation> SimulationManagerForIsolation::from_json_weakref(const nl::json& j){
    return j;
}

void exportAgentIsolationTools(py::module &m)
{
    using namespace pybind11::literals;
    py::class_<RulerForIsolation,RulerAccessor,std::shared_ptr<RulerForIsolation>>(m,"RulerForIsolation")
    .def(py::init<const std::string&,const std::string&,const std::string&>())
    .def("getStepScore",py::overload_cast<const std::string&>(&RulerForIsolation::getStepScore))
    .def("getStepScore",py::overload_cast<const std::shared_ptr<Agent>>(&RulerForIsolation::getStepScore))
    .def("getScore",py::overload_cast<const std::string&>(&RulerForIsolation::getScore))
    .def("getScore",py::overload_cast<const std::shared_ptr<Agent>>(&RulerForIsolation::getScore))
    .def_property_readonly("observables",[](const RulerForIsolation& v){return v.observables;})
    .def("isinstance",py::overload_cast<py::object>(&RulerForIsolation::isinstancePY))
    ;

    py::class_<FighterForIsolation,FighterAccessor,std::shared_ptr<FighterForIsolation>>(m,"FighterForIsolation")
    .def(py::init<const Factory&,const std::string&,const std::string&,const std::string&,const std::string&>())
    DEF_FUNC(FighterForIsolation,isAlive)
    DEF_FUNC(FighterForIsolation,getTeam)
    DEF_FUNC(FighterForIsolation,getGroup)
    DEF_FUNC(FighterForIsolation,getFullName)
    DEF_FUNC(FighterForIsolation,getName)
    .def_property_readonly("observables",[](const FighterForIsolation& v){return v.observables;})
    .def("isinstance",&FighterForIsolation::isinstancePY)
    DEF_FUNC(FighterForIsolation,setFlightControllerMode)
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&>(&FighterForIsolation::getRmax))
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>(&FighterForIsolation::getRmax))
    ;

    py::class_<DummyDependencyChecker,DependencyChecker,std::shared_ptr<DummyDependencyChecker>>(m,"DummyDependencyChecker")
    .def(py::init<>(&DummyDependencyChecker::create))
    .def("addDependency",py::overload_cast<const SimPhase&,const std::string&>(&DummyDependencyChecker::addDependency))
    .def("addDependency",py::overload_cast<const SimPhase&,std::shared_ptr<Asset>>(&DummyDependencyChecker::addDependency))
    DEF_STATIC_FUNC(DummyDependencyChecker,from_json_ref)
    .def_static("from_json_ref",[](const py::object& obj){return DummyDependencyChecker::from_json_ref(obj);})
    DEF_STATIC_FUNC(DummyDependencyChecker,from_json_weakref)
    .def_static("from_json_weakref",[](const py::object& obj){return DummyDependencyChecker::from_json_weakref(obj);})
    ;

    py::class_<SimulationManagerForIsolation,SimulationManagerAccessorForAgent,std::shared_ptr<SimulationManagerForIsolation>>(m,"SimulationManagerForIsolation")
    .def(py::init([](const py::object& config_){
        return SimulationManagerForIsolation::create(config_);
    }))
    //データパケットの変換
    DEF_FUNC(SimulationManagerForIsolation,setSimulationState)
    DEF_FUNC(SimulationManagerForIsolation,makeAgentState)
    //通信用インターフェース
    DEF_FUNC(SimulationManagerForIsolation,clear)
    DEF_FUNC(SimulationManagerForIsolation,initialize)
    DEF_FUNC(SimulationManagerForIsolation,action_space)
    DEF_FUNC(SimulationManagerForIsolation,observation_space)
    DEF_FUNC(SimulationManagerForIsolation,makeObs)
    DEF_FUNC(SimulationManagerForIsolation,deploy)
    DEF_FUNC(SimulationManagerForIsolation,validate)
    DEF_FUNC(SimulationManagerForIsolation,perceive)
    DEF_FUNC(SimulationManagerForIsolation,control)
    DEF_FUNC(SimulationManagerForIsolation,behave)
    //SimulationManagerとしてのインターフェース
    DEF_FUNC(SimulationManagerForIsolation,expired)
    DEF_FUNC(SimulationManagerForIsolation,getTime)
    DEF_FUNC(SimulationManagerForIsolation,getBaseTimeStep)
    DEF_FUNC(SimulationManagerForIsolation,getTickCount)
    DEF_FUNC(SimulationManagerForIsolation,getAgentInterval)
    DEF_FUNC(SimulationManagerForIsolation,getTeams)
    DEF_FUNC(SimulationManagerForIsolation,getRuler)
    DEF_FUNC(SimulationManagerForIsolation,copy)
    .def("generateUnmanagedChild",&SimulationManagerForIsolation::generateUnmanagedChild<Agent>)
    .def("generateUnmanagedChild",[](SimulationManagerForIsolation& v,const std::string& baseName,const std::string& modelName,const py::object& instanceConfig_){
        return v.generateUnmanagedChild<Agent>(baseName,modelName,instanceConfig_);
    })
    DEF_FUNC(SimulationManagerForIsolation,requestInvitationToCommunicationBuffer)
    DEF_FUNC(SimulationManagerForIsolation,to_json_ref)
    DEF_STATIC_FUNC(SimulationManagerForIsolation,from_json_ref)
    .def_static("from_json_ref",[](const py::object& obj){return SimulationManagerForIsolation::from_json_ref(obj);})
    DEF_STATIC_FUNC(SimulationManagerForIsolation,from_json_weakref)
    .def_static("from_json_weakref",[](const py::object& obj){return SimulationManagerForIsolation::from_json_weakref(obj);})
    ;
}

