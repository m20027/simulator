#pragma once
#include <ASRCAISim1/Common.h>
#include <memory>
#include <vector>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/Factory.h>
#include <ASRCAISim1/SimulationManager.h>
#include <ASRCAISim1/Agent.h>
#include <ASRCAISim1/Ruler.h>
#include <ASRCAISim1/Fighter.h>
#include <ASRCAISim1/Missile.h>
namespace py=pybind11;
namespace nl=nlohmann;

class SimulationManagerForIsolation;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(RulerForIsolation,RulerAccessor)
    friend class SimulationManagerForIsolation;
    public:
    RulerForIsolation(const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_);
    ~RulerForIsolation();
    template<class T>
    bool isinstance(){
        return Factory::issubclassof<T>(dummyRuler->getFactoryBaseName(),dummyRuler->getFactoryClassName());
    }
    bool isinstancePY(py::object cls);
    protected:
    RulerForIsolation(std::shared_ptr<Ruler> r,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_);
    void setData(const nl::json& data_);
    std::shared_ptr<Ruler> dummyRuler;
    std::string factoryBaseName,factoryClassName,factoryModelName;
};
DECLARE_CLASS_WITHOUT_TRAMPOLINE(FighterForIsolation,FighterAccessor)
    //FighterAccessorのインターフェースを再現するための仮実装。特にMissileの射程計算は強引な実装のため、
    //Fighter,Missileのカスタマイズ次第ではこちらも修正が必要になる。
    friend class SimulationManagerForIsolation;
    public:
    FighterForIsolation(const Factory& factory,const std::string& fullName_,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_);
    ~FighterForIsolation();
    bool isAlive() const;
    template<class T>
    bool isinstance(){
        return Factory::issubclassof<T>(dummyFighter->getFactoryBaseName(),dummyFighter->getFactoryClassName());
    }
    bool isinstancePY(py::object cls);
    void setFlightControllerMode(const std::string& ctrlName="");
    double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa);
    double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt);
    protected:
    FighterForIsolation(std::shared_ptr<Fighter> a,const Factory& factory,const std::string& fullName_,const std::string& factoryBaseName_,const std::string& factoryClassName_,const std::string& factoryModelName_);
    void setData(const nl::json& data_);
    std::shared_ptr<Fighter> dummyFighter;
    std::shared_ptr<Missile> dummyMissile;
    bool hasSetFlightControllerModeRequested;
    std::string flightControllerNameCommand;
};

class PYBIND11_EXPORT DummyDependencyChecker:public DependencyChecker{
    private:
    void clearReadiness();
    bool checkReadiness(const SimPhase &phase);
    void clearDependency();
    public:
    DummyDependencyChecker();
    ~DummyDependencyChecker();
    void addDependency(const SimPhase& phase,const std::string& fullName_);
    void addDependency(const SimPhase& phase,std::shared_ptr<Asset> asset);
    static std::shared_ptr<DummyDependencyChecker> from_json_ref(const nl::json& j);
    static std::weak_ptr<DummyDependencyChecker> from_json_weakref(const nl::json& j);
    static std::shared_ptr<DummyDependencyChecker> create();
};
class PYBIND11_EXPORT SimulationManagerForIsolation :public SimulationManagerAccessorForAgent{
    protected:
    Factory factory;
    nl::json agentModelConfig;
    std::map<std::string,std::shared_ptr<Agent>> agents;
    nl::json data;
    std::shared_ptr<RulerForIsolation> ruler;
    std::map<std::string,std::shared_ptr<FighterForIsolation>> parents;
    public:
    SimulationManagerForIsolation(const nl::json& agentModelConfig_);
    ~SimulationManagerForIsolation();
    static std::shared_ptr<SimulationManagerForIsolation> create(const nl::json& agentModelConfig_);
    //データパケットの変換
    void setSimulationState(const nl::json& data_);
    nl::json makeAgentState(const std::string& agentFullName);
    //通信用インターフェース
    void clear();
    nl::json initialize(const std::string& agentFullName,const nl::json& data_);
    py::tuple action_space(const std::string& agentFullName,const nl::json& data_);
    py::tuple observation_space(const std::string& agentFullName,const nl::json& data_);
    py::tuple makeObs(const std::string& agentFullName,const nl::json& data_);
    nl::json deploy(const std::string& agentFullName,const py::tuple& data_);
    nl::json validate(const std::string& agentFullName,const nl::json& data_);
    nl::json perceive(const std::string& agentFullName,const py::tuple& data_);
    nl::json control(const std::string& agentFullName,const nl::json& data_);
    nl::json behave(const std::string& agentFullName,const nl::json& data_);
    //SimulationManagerとしてのインターフェース
    bool expired() const noexcept;
    double getTime() const;
    double getBaseTimeStep() const;
    std::uint64_t getTickCount() const;
    std::uint64_t getAgentInterval() const;
    std::vector<std::string> getTeams() const;
    std::shared_ptr<RulerAccessor> getRuler() const;
    std::shared_ptr<SimulationManagerAccessorForAgent> copy();
    template<class T=Agent,
        std::enable_if_t<std::is_base_of_v<Agent,T>,std::nullptr_t> = nullptr
    >
    std::shared_ptr<T> generateUnmanagedChild(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
        throw std::runtime_error("An isolated agent cannot have children.");
        return std::shared_ptr<T>(nullptr);
    }
    bool requestInvitationToCommunicationBuffer(const std::string& bufferName,std::shared_ptr<Asset> asset);
    static std::shared_ptr<SimulationManagerForIsolation> from_json_ref(const nl::json& j);
    static std::weak_ptr<SimulationManagerForIsolation> from_json_weakref(const nl::json& j);
};

void exportAgentIsolationTools(py::module &m);
