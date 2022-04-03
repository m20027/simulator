#include "Factory.h"
#include <fstream>
#include "Utility.h"
#include "Asset.h"
#include "MassPointFighter.h"
#include "CoordinatedFighter.h"
#include "Missile.h"
#include "Sensor.h"
#include "Agent.h"
#include "R3InitialFighterAgent01.h"
#include "Callback.h"
#include "Ruler.h"
#include "R3BVRRuler01.h"
#include "Reward.h"
#include "R3BVRBasicReward01.h"
#include "Viewer.h"
std::map<std::string,std::map<std::string,Factory::creatorType>> Factory::creators;
std::map<std::string,std::map<std::string,nl::json>> Factory::defaultModelConfigs;

#ifdef FACTORY_ACCEPT_OVERWRITE
const bool Factory::acceptOverwriteDefaults=true;
#else
const bool Factory::acceptOverwriteDefaults=false;
#endif
Factory::Factory(){
    modelConfigs=defaultModelConfigs;
}
Factory::~Factory(){
}
void Factory::clear(){
    creators.clear();
    defaultModelConfigs.clear();
}
void Factory::reset(){
    modelConfigs=defaultModelConfigs;
}
std::shared_ptr<Entity> Factory::keepAlive(std::shared_ptr<Entity> pyObj){
    return pyObj;
}
void Factory::addClass(const std::string& baseName,const std::string& className,creatorType fn){
    if(acceptOverwriteDefaults){
        creators[baseName][className]=fn;
    }else{
        if(creators[baseName].count(className)==0){
            creators[baseName][className]=fn;
        }else{
            throw std::runtime_error(className+" is already registered in creators["+baseName+"].");
        }
    }
}
void Factory::addDefaultModel(const std::string& baseName,const std::string& modelName,const nl::json& modelConfig_){
    if(acceptOverwriteDefaults){
        defaultModelConfigs[baseName][modelName]=modelConfig_;
    }else{
        if(defaultModelConfigs[baseName].count(modelName)==0){
            defaultModelConfigs[baseName][modelName]=modelConfig_;
        }else{
            throw std::runtime_error(modelName+" is already registered in defaultModelConfigs["+baseName+"].");
        }
    }
}
void Factory::addDefaultModelsFromJson(const nl::json& j){
    assert(j.is_object());
    for(auto& base:j.items()){
        std::string baseName=base.key();
        for(auto& model:base.value().items()){
            addDefaultModel(baseName,model.key(),model.value());
        }
    }
}
void Factory::addDefaultModelsFromJsonFile(const std::string& filePath){
    std::ifstream ifs(filePath);
    nl::json j;
    ifs>>j;
    addDefaultModelsFromJson(j.at("Factory"));//requires "Factory" key in the root node.
}
void Factory::addModel(const std::string& baseName,const std::string& modelName,const nl::json& modelConfig_){
    modelConfigs[baseName][modelName]=modelConfig_;
}
void Factory::addModelsFromJson(const nl::json& j){
    assert(j.is_object());
    for(auto& base:j.items()){
        std::string baseName=base.key();
        for(auto& model:base.value().items()){
            addModel(baseName,model.key(),model.value());
        }
    }
}
void Factory::addModelsFromJsonFile(const std::string& filePath){
    std::ifstream ifs(filePath);
    nl::json j;
    ifs>>j;
    addModelsFromJson(j.at("Factory"));//requires "Factory" key in the root node.
}
nl::json Factory::getModelConfig() const{
    return modelConfigs;
}
void Factory::reconfigureModelConfig(const nl::json& j){
    nl::json tmp=modelConfigs;
    tmp.merge_patch(j);
    modelConfigs=tmp.get<std::map<std::string,std::map<std::string,nl::json>>>();
}
bool Factory::issubclassof(py::object baseCls,const std::string& derivedBaseName,const std::string& derivedClassName){
    auto dummy=createByClassName<Entity>(derivedBaseName,derivedClassName,nl::json(nullptr),nl::json(nullptr));
    return py::isinstance(py::cast(dummy),baseCls);
}
std::shared_ptr<Entity> Factory::createImpl(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
    nl::json modelConfig;
    std::string modelName_actual;
    if(modelConfigs.count(baseName)==0 || creators.count(baseName)==0){
        throw std::runtime_error(baseName+" is not registered as a base name in the Factory.");
    }
    if(modelConfigs[baseName].count(modelName)>0){
        modelConfig=modelConfigs[baseName][modelName];
        modelName_actual=modelName;
    }else{
        std::cout<<"Warning: "+modelName+" is not registered as a model name in the Factory with baseName="+baseName+"."<<std::endl;
        if(modelConfigs[baseName].count("Default")>0){
            std::cout<<"'Default' is used instead."<<std::endl;
            modelConfig=modelConfigs[baseName]["Default"];
            modelName_actual="Default";
        }else{
            throw std::runtime_error("'Default' is not registered as a model name in the Factory, either.");
        }
    }
    std::string className=modelConfig.at("class");//this may throw when 'class' is not in the config.
    if(creators[baseName].count(className)==0){
        throw std::runtime_error(className+" is not registered as a class name in the Factory.");
    }
    auto ret=creators[baseName][className](modelConfig.at("config"),instanceConfig_);
    ret->factoryBaseName=baseName;
    ret->factoryClassName=className;
    ret->factoryModelName=modelName_actual;
    return ret;
}
std::shared_ptr<Entity> Factory::createByClassNameImpl(const std::string& baseName,const std::string& className,const nl::json& modelConfig,const nl::json& instanceConfig){
    if(creators.count(baseName)==0){
        throw std::runtime_error(baseName+" is not registered as a base name in the Factory.");
    }
    if(creators[baseName].count(className)==0){
        throw std::runtime_error(className+" is not registered as a class name in the Factory.");
    }
    auto ret=creators[baseName][className](modelConfig,instanceConfig);
    ret->factoryBaseName=baseName;
    ret->factoryClassName=className;
    ret->factoryModelName="";
    return ret;
}

void Factory::dummyCreationTest(){
    for(auto&& e1:creators){
        std::string baseName=e1.first;
        for(auto&& e2:e1.second){
            std::string className=e2.first;
            try{
                auto dummy=createByClassName<Entity>(baseName,className,nl::json(nullptr),nl::json(nullptr));
                std::cout<<"Success: "<<baseName<<","<<className<<std::endl;
            }catch(...){
                std::cout<<"Failure: "<<baseName<<","<<className<<std::endl;
            }
        }
    }
}

void exportFactory(py::module &m)
{
    py::class_<Factory>(m,"Factory")
    .def_static("keepAlive",&Factory::keepAlive,py::keep_alive<0,1>())
    DEF_STATIC_FUNC(Factory,clear)
    DEF_FUNC(Factory,reset)
    DEF_STATIC_FUNC(Factory,addClass)
    .def_static("addClass",[](const std::string& baseName,const std::string& className,std::function<std::shared_ptr<Entity>(const py::object&,const py::object&)> fn){
        return Factory::addClass(baseName,className,fn);
    })
    DEF_STATIC_FUNC(Factory,addDefaultModel)
    .def_static("addDefaultModel",[](const std::string& baseName,const std::string& modelName,const py::object& modelConfig_){
        return Factory::addDefaultModel(baseName,modelName,modelConfig_);
    })
    DEF_STATIC_FUNC(Factory,addDefaultModelsFromJson)
    .def_static("addDefaultModelsFromJson",[](const py::object& j){
        return Factory::addDefaultModelsFromJson(j);
    })
    DEF_STATIC_FUNC(Factory,addDefaultModelsFromJsonFile)
    DEF_FUNC(Factory,addModel)
    .def("addModel",[](Factory& v,const std::string& baseName,const std::string& modelName,const py::object& modelConfig_){
        return v.addDefaultModel(baseName,modelName,modelConfig_);
    })
    DEF_FUNC(Factory,addModelsFromJson)
    .def("addModelsFromJson",[](Factory& v,const py::object& j){
        return v.addDefaultModelsFromJson(j);
    })
    DEF_FUNC(Factory,addModelsFromJsonFile)
    DEF_FUNC(Factory,getModelConfig)
    DEF_FUNC(Factory,reconfigureModelConfig)
    .def("reconfigureModelConfig",[](Factory& v,const py::object& j){
        return v.reconfigureModelConfig(j);
    })
    .def_static("issubclassof",[](py::object baseCls,const std::string& derivedBaseName,const std::string& derivedClassName){
        return Factory::issubclassof(baseCls,derivedBaseName,derivedClassName);
    })
    DEF_STATIC_FUNC(Factory,dummyCreationTest)
    //.def("create",&Factory::create<Entity>)
    ;
}

void setupBuiltIns(){
    FACTORY_ADD_CLASS(PhysicalAsset,MassPointFighter)
    FACTORY_ADD_CLASS(Controller,Fighter::SensorDataSharer)
    FACTORY_ADD_CLASS(Controller,Fighter::SensorDataSanitizer)
    FACTORY_ADD_CLASS(Controller,Fighter::OtherDataSharer)
    FACTORY_ADD_CLASS(Controller,Fighter::OtherDataSanitizer)
    FACTORY_ADD_CLASS(Controller,Fighter::HumanIntervention)
    FACTORY_ADD_CLASS(Controller,Fighter::WeaponController)
    FACTORY_ADD_CLASS(Controller,MassPointFighter::FlightController)
    FACTORY_ADD_CLASS(PhysicalAsset,CoordinatedFighter)
    FACTORY_ADD_CLASS(Controller,CoordinatedFighter::FlightController)
    FACTORY_ADD_CLASS(PhysicalAsset,Propulsion)
    FACTORY_ADD_CLASS(PhysicalAsset,Missile)
    FACTORY_ADD_CLASS(Controller,PropNav)
    FACTORY_ADD_CLASS(PhysicalAsset,Sensor)
    FACTORY_ADD_CLASS(PhysicalAsset,AircraftRadar)
    FACTORY_ADD_CLASS(PhysicalAsset,MWS)
    FACTORY_ADD_CLASS(PhysicalAsset,MissileSensor)
    FACTORY_ADD_CLASS(Agent,Agent)
    FACTORY_ADD_CLASS(Agent,ExpertWrapper)
    FACTORY_ADD_CLASS(Agent,MultiPortCombiner)
    FACTORY_ADD_CLASS(Agent,SingleAssetAgent)
    FACTORY_ADD_CLASS(Agent,R3InitialFighterAgent01)
    FACTORY_ADD_CLASS(Callback,Callback)
    FACTORY_ADD_CLASS(Ruler,Ruler)
    FACTORY_ADD_CLASS(Ruler,R3BVRRuler01)
    FACTORY_ADD_CLASS(Reward,Reward)
    FACTORY_ADD_CLASS(Reward,AgentReward)
    FACTORY_ADD_CLASS(Reward,TeamReward)
    FACTORY_ADD_CLASS(Reward,ScoreReward)
    FACTORY_ADD_CLASS(Reward,R3BVRBasicReward01)
    FACTORY_ADD_CLASS(Viewer,Viewer)
    auto atexit=py::module_::import("atexit");
    atexit.attr("register")(py::cpp_function([](){
        Factory::clear();
    }));
}