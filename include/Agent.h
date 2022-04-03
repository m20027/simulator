#pragma once
#include <map>
#include <pybind11/pybind11.h>
#include "Asset.h"
#include "Utility.h"

namespace py=pybind11;
namespace nl=nlohmann;
class SimulationManager;
class SimulationManagerAccessorForAgent;
class PhysicalAssetAccessor;

DECLARE_CLASS_WITH_TRAMPOLINE(Agent,Asset)
    friend class SimulationManager;
    public:
    std::shared_ptr<SimulationManagerAccessorForAgent> manager;
    std::string name,type,model,policy;
    std::map<std::string,std::shared_ptr<PhysicalAssetAccessor>> parents;
    public:
    std::map<std::string,bool> readiness;
    //constructors & destructor
    Agent(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Agent();
    //functions
    virtual bool isAlive() const;
    virtual std::string getTeam() const;
    virtual std::string getGroup() const;
    virtual std::string getName() const;
    virtual std::string getFullName() const;
    virtual std::string repr() const;
    virtual void validate();
    void setDependency();//disable for Agent. Agent's dependency should be set by the parent PhysicalAsset.
    virtual py::object observation_space();
    virtual py::object makeObs();
    virtual py::object action_space();
    virtual void deploy(py::object action);
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual py::object convertActionFromAnother(const nl::json& decision,const nl::json& command);
};

DECLARE_TRAMPOLINE(Agent)
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
    virtual std::string repr() const override{
        PYBIND11_OVERRIDE_NAME(std::string,Base,"__repr__",repr);
    }
    virtual py::object observation_space() override{
        PYBIND11_OVERRIDE(py::object,Base,observation_space);
    }
    virtual py::object makeObs() override{
        PYBIND11_OVERRIDE(py::object,Base,makeObs);
    }
    virtual py::object action_space() override{
        PYBIND11_OVERRIDE(py::object,Base,action_space);
    }
    virtual void deploy(py::object action) override{
        PYBIND11_OVERRIDE(void,Base,deploy,action);
    }
    virtual py::object convertActionFromAnother(const nl::json& decisions,const nl::json& outputs) override{
        PYBIND11_OVERRIDE(py::object,Base,convertActionFromAnother,decisions,outputs);
    }
};

DECLARE_CLASS_WITH_TRAMPOLINE(ExpertWrapper,Agent)
    public:
    std::string imitatorModelName,expertModelName,expertPolicyName,whichOutput,trajectoryIdentifier;
    std::shared_ptr<Agent> imitator,expert;
    py::object imitatorObs,imitatorAction;
    bool isInternal,hasImitatorDeployed;
    //constructors & destructor
    ExpertWrapper(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~ExpertWrapper();
    //functions
    virtual std::string repr() const;
    virtual void validate();
    virtual py::object observation_space();
    virtual py::object makeObs();
    virtual py::object action_space();
    virtual py::object imitator_observation_space();
    virtual py::object imitator_action_space();
    virtual void deploy(py::object action);
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
};
DECLARE_TRAMPOLINE(ExpertWrapper)
    //virtual functions
    virtual py::object imitator_observation_space(){
        PYBIND11_OVERRIDE(py::object,Base,imitator_observation_space);
    }
    virtual py::object imitator_action_space(){
        PYBIND11_OVERRIDE(py::object,Base,imitator_action_space);
    }
};

DECLARE_CLASS_WITH_TRAMPOLINE(MultiPortCombiner,Agent)
    //複数のAgentを組み合わせて一つのAgentとして扱うためのベースクラス
    //一つにまとめた後のObservationとActionはユーザーが定義し、
    //派生クラスにおいてmakeObs,actionSplitter,observation_space,action_spaceの4つをオーバーライドする必要がある。
    public:
    std::map<std::string,std::map<std::string,std::string>> ports;
    std::map<std::string,std::shared_ptr<Agent>> children;
    //constructors & destructor
    MultiPortCombiner(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~MultiPortCombiner();
    //functions
    virtual void validate();
    virtual py::object observation_space();
    virtual py::object makeObs();
    virtual py::object action_space();
    virtual std::map<std::string,py::object> actionSplitter(py::object action);
    virtual void deploy(py::object action);
    virtual void perceive(bool inReset);
    virtual void control();
};
DECLARE_TRAMPOLINE(MultiPortCombiner)
    virtual std::map<std::string,py::object> actionSplitter(py::object action){
        typedef std::map<std::string,py::object> retType;
        PYBIND11_OVERRIDE(retType,Base,actionSplitter,action);
    }
};

DECLARE_CLASS_WITHOUT_TRAMPOLINE(SingleAssetAgent,Agent)
    public:
    std::shared_ptr<PhysicalAssetAccessor> parent;
    std::string port;
    public:
    //constructors & destructor
    SingleAssetAgent(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~SingleAssetAgent();
};

void exportAgent(py::module &m);
