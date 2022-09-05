#pragma once
#include <ASRCAISim1/Agent.h>

DECLARE_CLASS_WITH_TRAMPOLINE(AgentDelegatorBase,Agent)
    public:
    std::map<std::string,std::string> parentFullNameToPort;
    public:
    AgentDelegatorBase(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~AgentDelegatorBase();
	virtual nl::json makeSimulationState();
	virtual void parseAgentState(const nl::json& data_);
	nl::json data;
};
DECLARE_TRAMPOLINE(AgentDelegatorBase)
    //virtual functions
    virtual nl::json makeSimulationState(){
        PYBIND11_OVERRIDE(nl::json,Base,makeSimulationState);
    }
    virtual void parseAgentState(const nl::json& data){
        PYBIND11_OVERRIDE(void,Base,parseAgentState,data);
    }
};

void exportAgentDelegatorBase(py::module &m);
