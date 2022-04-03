#pragma once
#include <map>
#include <pybind11/pybind11.h>
#include "Asset.h"
#include "Utility.h"

namespace py=pybind11;
namespace nl=nlohmann;
class SimulationManager;
class SimulationManagerAccessorForPhysicalAsset;

DECLARE_CLASS_WITH_TRAMPOLINE(Controller,Asset)
    friend class SimulationManager;
    public:
    std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> manager;
    std::string fullName,team,group,name;
    std::weak_ptr<Asset> parent;
    //constructors & destructor
    Controller(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Controller();
    //functions
    virtual bool isAlive() const;
    virtual std::string getTeam() const;
    virtual std::string getGroup() const;
    virtual std::string getName() const;
    virtual std::string getFullName() const;
    virtual void validate();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual void kill();
};

DECLARE_TRAMPOLINE(Controller)
    virtual bool isAlive() const override{
        PYBIND11_OVERRIDE(bool,Base,isAlive);
    }
    virtual std::string getTeam() const override{
        PYBIND11_OVERRIDE(std::string,Base,getTeam);
    }
    virtual std::string getGroup() const override{
        PYBIND11_OVERRIDE(std::string,Base,getGroup);
    }
    virtual std::string getName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getName);
    }
    virtual std::string getFullName() const override{
        PYBIND11_OVERRIDE(std::string,Base,getFullName);
    }
};

void exportController(py::module &m);
