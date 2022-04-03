#pragma once
#include <pybind11/pybind11.h>
#include "Entity.h"
class SimulationManager;
class SimulationManagerAccessorForCallback;

namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITH_TRAMPOLINE(Callback,Entity)
    friend class SimulationManager;
    public:
    std::shared_ptr<SimulationManagerAccessorForCallback> manager;
    bool acceptReconfigure;
    std::string name;
    public:
    //constructors & destructor
    Callback(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Callback();
    //functions
    virtual std::string getName() const;
    virtual void onEpisodeBegin();
    virtual void onStepBegin();
    virtual void onInnerStepBegin();
    virtual void onInnerStepEnd();
    virtual void onStepEnd();
    virtual void onEpisodeEnd();
};

DECLARE_TRAMPOLINE(Callback)
    virtual std::string getName() const{
        PYBIND11_OVERRIDE(std::string,Base,getName);
    }
    virtual void onEpisodeBegin() override{
        PYBIND11_OVERRIDE(void,Base,onEpisodeBegin);
    }
    virtual void onStepBegin() override{
        PYBIND11_OVERRIDE(void,Base,onStepBegin);
    }
    virtual void onInnerStepBegin() override{
        PYBIND11_OVERRIDE(void,Base,onInnerStepBegin);
    }
    virtual void onInnerStepEnd() override{
        PYBIND11_OVERRIDE(void,Base,onInnerStepEnd);
    }
    virtual void onStepEnd() override{
        PYBIND11_OVERRIDE(void,Base,onStepEnd);
    }
    virtual void onEpisodeEnd() override{
        PYBIND11_OVERRIDE(void,Base,onEpisodeEnd);
    }
};

void exportCallback(py::module &m);
