#pragma once
#include "Common.h"
#include <iostream>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>

namespace py=pybind11;
namespace nl=nlohmann;
class Entity;

class PYBIND11_EXPORT Factory{//Only Entity
    public:
    typedef std::function<std::shared_ptr<Entity>(const nl::json&,const nl::json&)> creatorType;
    Factory();
    ~Factory();
    static void clear();
    void reset();
    static void addClass(const std::string& baseName,const std::string& className,creatorType fn);
    static void addDefaultModel(const std::string& baseName,const std::string& modelName,const nl::json& modelConfig_);
    static void addDefaultModelsFromJson(const nl::json& j);
    static void addDefaultModelsFromJsonFile(const std::string& filePath);
    void addModel(const std::string& baseName,const std::string& modelName,const nl::json& modelConfig_);
    void addModelsFromJson(const nl::json& j);
    void addModelsFromJsonFile(const std::string& filePath);
    nl::json getModelConfig() const;
    void reconfigureModelConfig(const nl::json& j);
    static std::shared_ptr<Entity> keepAlive(std::shared_ptr<Entity> pyObj);//In order to keep other object of a class which is derived from Entity in python object alive as a child of this.
    template<class T>
    std::shared_ptr<T> create(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
        return std::dynamic_pointer_cast<T>(createImpl(baseName,modelName,instanceConfig_));
    }
    template<class T>
    static std::shared_ptr<T> createByClassName(const std::string& baseName,const std::string& className,const nl::json& modelConfig,const nl::json& instanceConfig){
        return std::dynamic_pointer_cast<T>(createByClassNameImpl(baseName,className,modelConfig,instanceConfig));
    }
    //C++側での継承関係チェック
    template<class Base>
    static bool issubclassof(const std::string& derivedBaseName,const std::string& derivedClassName){
        auto dummy=createByClassName<Entity>(derivedBaseName,derivedClassName,nl::json(nullptr),nl::json(nullptr));
        std::shared_ptr<Base> t=std::dynamic_pointer_cast<Base>(dummy);
        return (bool)t;
    }
    //Python側クラスオブジェクトによる継承関係チェック
    static bool issubclassof(py::object baseCls,const std::string& derivedBaseName,const std::string& derivedClassName);
    static void dummyCreationTest();
    protected:
    std::shared_ptr<Entity> createImpl(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_);
    static std::shared_ptr<Entity> createByClassNameImpl(const std::string& baseName,const std::string& className,const nl::json& modelConfig,const nl::json& instanceConfig);
    private:
    static std::map<std::string,std::map<std::string,creatorType>> creators;
    static std::map<std::string,std::map<std::string,nl::json>> defaultModelConfigs;
    std::map<std::string,std::map<std::string,nl::json>> modelConfigs;
    static const bool acceptOverwriteDefaults;
};
#define FACTORY_ADD_CLASS(baseName,className) Factory::addClass(#baseName,#className,&className::create<typename className::Type>);
#define FACTORY_ADD_CLASS_NAME(baseName,className,registerClassName) Factory::addClass(#baseName,registerClassName,&className::create<typename className::Type>);

void exportFactory(py::module &m);
void setupBuiltIns();
