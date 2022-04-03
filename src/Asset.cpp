#include "Asset.h"
#include "Utility.h"
#include "SimulationManager.h"
#include "Agent.h"
#include "CommunicationBuffer.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace util;
Asset::Asset(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Entity(modelConfig_,instanceConfig_),observables(nl::json::object()),commands(nl::json::object()){
    if(isDummy){return;}
    uuid=boost::uuids::random_generator()();
    dependencyChecker=instanceConfig.at("dependencyChecker");
    firstTick[SimPhase::VALIDATE]=0;//meaningless but defined for same interface
    firstTick[SimPhase::PERCEIVE]=1;//reset at t=0 is not regarded as the "first time."
    firstTick[SimPhase::CONTROL]=0;
    firstTick[SimPhase::BEHAVE]=0;
    interval[SimPhase::VALIDATE]=std::numeric_limits<std::uint64_t>::max();
    interval[SimPhase::PERCEIVE]=1;
    interval[SimPhase::CONTROL]=1;
    interval[SimPhase::BEHAVE]=1;
}
Asset::~Asset(){}
void Asset::addCommunicationBuffer(std::shared_ptr<CommunicationBuffer> buffer){
    communicationBuffers[buffer->getName()]=buffer;
}
void Asset::makeChildren(){
}
void Asset::validate(){

}
void Asset::setDependency(){
    //Inform children parent's sequential process order.
    //Use dependencyChecker->addDependency to add dependency.
}
void Asset::perceive(bool inReset){
}
void Asset::control(){
}
void Asset::behave(){
}
void Asset::kill(){
}
std::shared_ptr<AssetAccessor> Asset::getAccessor(){
    if(!accessor){
        accessor=std::make_shared<AssetAccessor>(getShared<Asset>(this->shared_from_this()));
    }
    return accessor;
}
nl::json AssetAccessor::dummy=nl::json();
std::string AssetAccessor::getFactoryBaseName() const{
    return asset.lock()->getFactoryBaseName();
}
std::string AssetAccessor::getFactoryClassName() const{
    return asset.lock()->getFactoryClassName();
}
std::string AssetAccessor::getFactoryModelName() const{
    return asset.lock()->getFactoryModelName();
}
bool AssetAccessor::isAlive() const{
    return asset.lock()->isAlive();
}
std::string AssetAccessor::getTeam() const{
    return asset.lock()->getTeam();
}
std::string AssetAccessor::getGroup() const{
    return asset.lock()->getGroup();
}
std::string AssetAccessor::getFullName() const{
    return asset.lock()->getFullName();
}
std::string AssetAccessor::getName() const{
    return asset.lock()->getName();
}
AssetAccessor::AssetAccessor(std::shared_ptr<Asset> a)
:observables(a ? a->observables : dummy){
    asset=a;
}
AssetAccessor::~AssetAccessor(){
}
bool AssetAccessor::isinstancePY(py::object cls){
    return py::isinstance(py::cast(asset),cls);
}

void exportAsset(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Asset)
    DEF_FUNC(Asset,getTeam)
    DEF_FUNC(Asset,getGroup)
    DEF_FUNC(Asset,getFullName)
    DEF_FUNC(Asset,getName)
    DEF_FUNC(Asset,makeChildren)
    DEF_FUNC(Asset,validate)
    DEF_FUNC(Asset,setDependency)
    DEF_FUNC(Asset,perceive)
    DEF_FUNC(Asset,control)
    DEF_FUNC(Asset,behave)
    DEF_FUNC(Asset,kill)
    DEF_FUNC(Asset,getAccessor)
    DEF_READWRITE(Asset,communicationBuffers)
    DEF_READWRITE(Asset,dependencyChecker)
    DEF_READWRITE(Asset,observables)
    DEF_READWRITE(Asset,commands)
    ;
    EXPOSE_BASE_CLASS_WITHOUT_INIT(AssetAccessor)
    .def(py::init<std::shared_ptr<Asset>>())
    DEF_FUNC(AssetAccessor,getFactoryBaseName)
    DEF_FUNC(AssetAccessor,getFactoryClassName)
    DEF_FUNC(AssetAccessor,getFactoryModelName)
    DEF_FUNC(AssetAccessor,isAlive)
    DEF_FUNC(AssetAccessor,getTeam)
    DEF_FUNC(AssetAccessor,getGroup)
    DEF_FUNC(AssetAccessor,getFullName)
    DEF_FUNC(AssetAccessor,getName)
    .def_property_readonly("observables",[](const AssetAccessor& v){return v.observables;})
    .def("isinstance",&AssetAccessor::isinstancePY)
    ;
}