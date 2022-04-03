#include "Entity.h"
#include "Utility.h"
#include "SimulationManager.h"
using namespace util;
Entity::Entity(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:modelConfig(modelConfig_),instanceConfig(instanceConfig_),isDummy(modelConfig_.is_null() && instanceConfig_.is_null()){
    if(isDummy){return;}
    unsigned int seed_;
    try{
        seed_=instanceConfig.at("seed");
    }catch(...){
        seed_=std::random_device()();
    }
    seed(seed_);
    factoryBaseName="";//Factoryにより生成された場合のみFactoryにより外から設定される。
    factoryClassName="";//Factoryにより生成された場合のみFactoryにより外から設定される。
    factoryModelName="";//Factoryにより生成された場合のみFactoryにより外から設定される。
}
Entity::~Entity(){
}
void Entity::seed(const unsigned int& seed_){
    randomGen.seed(seed_);
}
std::string Entity::getFactoryBaseName() const{
    //FactoryにおけるbaseNameを返す。
    return factoryBaseName;
}
std::string Entity::getFactoryClassName() const{
    //Factoryにおけるクラス名を返す。
    return factoryClassName;
}
std::string Entity::getFactoryModelName() const{
    //Factoryにおけるモデル名を返す。
    return factoryModelName;
}
std::uint64_t Entity::getFirstTick(const SimPhase& phase) const{
    return firstTick.at(phase);
}
std::uint64_t Entity::getNextTick(const SimPhase& phase,const std::uint64_t now){
    if(std::numeric_limits<std::uint64_t>::max()-now>=interval.at(phase)){
        return now+interval.at(phase);
    }else{
        return std::numeric_limits<std::uint64_t>::max();
    }
}
bool Entity::isSame(std::shared_ptr<Entity> other){
    return this==other.get();
}
void Entity::setChild(std::shared_ptr<Entity> child_){
    child=child_;
}
void Entity::setWeakChild(std::shared_ptr<Entity> child_){
    weakChild=child_;
}
std::shared_ptr<Entity> Entity::getChild(){
    return child;
}
py::object Entity::getWeakChildPY(){
    py::module_ weakref=py::module_::import("weakref");
    return weakref.attr("proxy")(py::cast(weakChild.lock()));
}
nl::json Entity::to_json_ref(){
    return this->shared_from_this();
    //nl::json ret=this->shared_from_this();
    //return std::move(ret);
}
void exportEntity(py::module &m)
{
    EXPOSE_BASE_CLASS(Entity)
    DEF_FUNC(Entity,seed)
    DEF_FUNC(Entity,getFactoryBaseName)
    DEF_FUNC(Entity,getFactoryClassName)
    DEF_FUNC(Entity,getFactoryModelName)
    DEF_FUNC(Entity,getFirstTick)
    DEF_FUNC(Entity,getNextTick)
    DEF_FUNC(Entity,isSame)
    DEF_FUNC(Entity,setChild)
    DEF_FUNC(Entity,setWeakChild)
    DEF_FUNC(Entity,getChild)
    DEF_FUNC(Entity,getWeakChildPY)
    DEF_FUNC(Entity,to_json_ref)
    .def_static("from_json_ref",&Entity::from_json_ref<Entity>)
    .def_static("from_json_ref",[](const py::object& obj){return Entity::from_json_ref(obj);})
    .def_static("from_json_weakref",&Entity::from_json_weakref<Entity>)
    .def_static("from_json_weakref",[](const py::object& obj){return Entity::from_json_weakref(obj);})
    DEF_READONLY(Entity,isDummy)
    DEF_READONLY(Entity,firstTick)
    DEF_READONLY(Entity,interval)
    DEF_READWRITE(Entity,modelConfig)
    DEF_READWRITE(Entity,instanceConfig)
    DEF_READONLY(Entity,randomGen)
    ;
}

