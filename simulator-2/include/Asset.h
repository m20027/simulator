#pragma once
#include <map>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <boost/uuid/uuid.hpp>
#include "Quaternion.h"
#include "MathUtility.h"
#include "Entity.h"
namespace py=pybind11;
namespace nl=nlohmann;
class DependencyChecker;
class SimulationManager;
class SimulationManagerAccessorForPhysicalAsset;
class Agent;
class Controller;
class CommunicationBuffer;
class AssetAccessor;

DECLARE_CLASS_WITH_TRAMPOLINE(Asset,Entity)
    private:
    friend class SimulationManager;
    friend class DependencyChecker;
    protected:
    std::shared_ptr<AssetAccessor> accessor;
    public:
    boost::uuids::uuid uuid;
    std::map<std::string,std::weak_ptr<CommunicationBuffer>> communicationBuffers;
    std::shared_ptr<DependencyChecker> dependencyChecker;
    nl::json observables,commands;
    //constructors & destructor
    Asset(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Asset();
    //functions
    virtual bool isAlive() const=0;
    virtual std::string getTeam() const=0;
    virtual std::string getGroup() const=0;
    virtual std::string getName() const=0;
    virtual std::string getFullName() const=0;
    virtual void addCommunicationBuffer(std::shared_ptr<CommunicationBuffer> buffer);
    virtual void makeChildren();
    virtual void validate();
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual void kill();
    virtual std::shared_ptr<AssetAccessor> getAccessor();
};

DECLARE_BASE_CLASS(AssetAccessor)
    friend class SimulationManager;
    public:
    AssetAccessor(std::shared_ptr<Asset> a);
    virtual ~AssetAccessor();
    virtual std::string getFactoryBaseName() const;//FactoryにおけるbaseNameを返す。
    virtual std::string getFactoryClassName() const;//Factoryにおけるクラス名を返す。
    virtual std::string getFactoryModelName() const;//Factoryにおけるモデル名を返す。
    virtual bool isAlive() const;
    virtual std::string getTeam() const;
    virtual std::string getGroup() const;
    virtual std::string getFullName() const;
    virtual std::string getName() const;
    const nl::json& observables;
    template<class T>
    bool isinstance(){
        return util::isinstance<T>(asset);
    }
    virtual bool isinstancePY(py::object cls);
    protected:
    static nl::json dummy;
    std::weak_ptr<Asset> asset;
};

DECLARE_TRAMPOLINE(Asset)
    virtual bool isAlive() const override{
        PYBIND11_OVERRIDE_PURE(bool,Base,isAlive);
    }
    virtual std::string getTeam() const override{
        PYBIND11_OVERRIDE_PURE(std::string,Base,getTeam);
    }
    virtual std::string getGroup() const override{
        PYBIND11_OVERRIDE_PURE(std::string,Base,getGroup);
    }
    virtual std::string getFullName() const override{
        PYBIND11_OVERRIDE_PURE(std::string,Base,getFullName);
    }
    virtual std::string getName() const override{
        PYBIND11_OVERRIDE_PURE(std::string,Base,getName);
    }
    virtual void addCommunicationBuffer(std::shared_ptr<CommunicationBuffer> buffer){
        PYBIND11_OVERRIDE(void,Base,addCommunicationBuffer,buffer);
    }
    virtual void makeChildren() override{
        PYBIND11_OVERRIDE(void,Base,makeChildren);
    }
    virtual void validate() override{
        PYBIND11_OVERRIDE(void,Base,validate);
    }
    virtual void setDependency() override{
        PYBIND11_OVERRIDE(void,Base,setDependency);
    }
    virtual void perceive(bool inReset) override{
        PYBIND11_OVERRIDE(void,Base,perceive,inReset);
    }
    virtual void control() override{
        PYBIND11_OVERRIDE(void,Base,control);
    }
    virtual void behave() override{
        PYBIND11_OVERRIDE(void,Base,behave);
    }
    virtual void kill() override{
        PYBIND11_OVERRIDE(void,Base,kill);
    }
    virtual std::shared_ptr<AssetAccessor> getAccessor() override{
        PYBIND11_OVERRIDE(std::shared_ptr<AssetAccessor>,Base,getAccessor);
    }
};

DECLARE_BASE_TRAMPOLINE(AssetAccessor)
    virtual std::string getFactoryBaseName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getFactoryBaseName);
    }
    virtual std::string getFactoryClassName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getFactoryClassName);
    }
    virtual std::string getFactoryModelName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getFactoryModelName);
    }
    virtual bool isAlive() const override{
        PYBIND11_OVERRIDE(bool,Base,isAlive);
    }
    virtual std::string getTeam() const override{
        PYBIND11_OVERRIDE(std::string,Base,getTeam);
    }
    virtual std::string getGroup() const override{
        PYBIND11_OVERRIDE(std::string,Base,getGroup);
    }
    virtual std::string getFullName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getFullName);
    }
    virtual std::string getName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getName);
    }
    virtual bool isinstancePY(py::object cls) override{
        PYBIND11_OVERRIDE(bool,Base,isinstancePY,cls);
    }
};

void exportAsset(py::module &m);