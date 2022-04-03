#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <pybind11/pybind11.h>
#include "Callback.h"

namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITH_TRAMPOLINE(Viewer,Callback)
    public:
    bool isValid;
    std::string name;
    //constructors & destructor
    Viewer(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Viewer();
    //functions
    virtual void validate();
    virtual void display();
    virtual void close();
    virtual void onEpisodeBegin();
    virtual void onInnerStepBegin();
    virtual void onInnerStepEnd();
};
DECLARE_TRAMPOLINE(Viewer)
    virtual void validate() override{
        PYBIND11_OVERRIDE(void,Base,validate);
    }
    virtual void display() override{
        PYBIND11_OVERRIDE(void,Base,display);
    }
    virtual void close() override{
        PYBIND11_OVERRIDE(void,Base,close);
    }
};
void exportViewer(py::module &m);