#pragma once
//#define EIGEN_MPL2_ONLY
#include <map>
//#include <memory>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include "Quaternion.h"
#include "MotionState.h"
#include "MathUtility.h"
#include "Asset.h"
namespace py=pybind11;
namespace nl=nlohmann;
class SimulationManager;
class SimulationManagerAccessorForPhysicalAsset;
class Agent;
class Controller;

DECLARE_CLASS_WITH_TRAMPOLINE(PhysicalAsset,Asset)
    private:
    friend class SimulationManager;
    virtual void setAgent(std::weak_ptr<Agent> agent,const std::string &port);
    public:
    std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> manager;
    std::weak_ptr<PhysicalAsset> parent;//can be nullptr
    std::string fullName,team,group,name;
    std::map<std::string,bool> readiness;
    bool isAlive_;
    //Agent関係
    bool hasAgent;
    std::weak_ptr<Agent> agent;
    std::map<std::string,std::weak_ptr<PhysicalAsset>> children;
    std::map<std::string,std::weak_ptr<Controller>> controllers;
    std::string port;
    //座標系関係
    bool isBoundToParent;
    MotionState motion;//isBoundToParent==trueならparent系、falseなら慣性系
    Eigen::Vector3d pos_prev,vel_prev;//isBoundToParent==trueならparent系、falseなら慣性系
    public:
    //constructors & destructor
    PhysicalAsset(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~PhysicalAsset();
    //functions
    virtual bool isAlive() const;
    virtual std::string getTeam() const;
    virtual std::string getGroup() const;
    virtual std::string getFullName() const;
    virtual std::string getName() const;
    virtual void makeChildren();
    virtual void validate();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual void kill();
    //座標変換
    //(1)相対位置ベクトルの変換
    virtual Eigen::Vector3d relBtoI(const Eigen::Vector3d &v) const;//self系⇛慣性系
    virtual Eigen::Vector3d relItoB(const Eigen::Vector3d &v) const;//慣性系⇛self系
    virtual Eigen::Vector3d relBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    virtual Eigen::Vector3d relPtoB(const Eigen::Vector3d &v) const;//paernt系⇛self系
    //(2)絶対位置ベクトルの変換
    virtual Eigen::Vector3d absBtoI(const Eigen::Vector3d &v) const;//self系⇛慣性系
    virtual Eigen::Vector3d absItoB(const Eigen::Vector3d &v) const;//慣性系⇛self系
    virtual Eigen::Vector3d absBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    virtual Eigen::Vector3d absPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    //(3)速度ベクトルの変換
    virtual Eigen::Vector3d velBtoI(const Eigen::Vector3d &v) const;//self系⇛慣性系
    virtual Eigen::Vector3d velItoB(const Eigen::Vector3d &v) const;//慣性系⇛self系
    virtual Eigen::Vector3d velBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    virtual Eigen::Vector3d velPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    virtual Eigen::Vector3d velBtoI(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//self系⇛慣性系
    virtual Eigen::Vector3d velItoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//慣性系⇛self系
    virtual Eigen::Vector3d velBtoP(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//self系⇛parent系
    virtual Eigen::Vector3d velPtoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const;//parent系⇛self系
    //(4)角速度ベクトルの変換
    virtual Eigen::Vector3d omegaBtoI(const Eigen::Vector3d &v) const;//self系⇛慣性系
    virtual Eigen::Vector3d omegaItoB(const Eigen::Vector3d &v) const;//慣性系⇛self系
    virtual Eigen::Vector3d omegaBtoP(const Eigen::Vector3d &v) const;//self系⇛parent系
    virtual Eigen::Vector3d omegaPtoB(const Eigen::Vector3d &v) const;//parent系⇛self系
    //自身の座標情報
    virtual Eigen::Vector3d posI() const;
    virtual Eigen::Vector3d posP() const;
    virtual Eigen::Vector3d velI() const;
    virtual Eigen::Vector3d velP() const;
    virtual Quaternion qI() const;
    virtual Quaternion qP() const;
    virtual Eigen::Vector3d omegaI() const;
    virtual Eigen::Vector3d omegaP() const;
    virtual std::shared_ptr<AssetAccessor> getAccessor();
};

DECLARE_TRAMPOLINE(PhysicalAsset)
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
    virtual Eigen::Vector3d relBtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relBtoI,v);
    }
    virtual Eigen::Vector3d relItoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relItoB,v);
    }
    virtual Eigen::Vector3d relBtoP(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relBtoP,v);
    }
    virtual Eigen::Vector3d relPtoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relPtoB,v);
    }
    virtual Eigen::Vector3d absBtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absBtoI,v);
    }
    virtual Eigen::Vector3d absItoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absItoB,v);
    }
    virtual Eigen::Vector3d absBtoP(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absBtoP,v);
    }
    virtual Eigen::Vector3d absPtoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absPtoB,v);
    }
    virtual Eigen::Vector3d velBtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velBtoI,v);
    }
    virtual Eigen::Vector3d velItoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velItoB,v);
    }
    virtual Eigen::Vector3d velBtoP(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velBtoP,v);
    }
    virtual Eigen::Vector3d velPtoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velPtoB,v);
    }
    virtual Eigen::Vector3d velBtoI(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velBtoI,v,r);
    }
    virtual Eigen::Vector3d velItoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velItoB,v,r);
    }
    virtual Eigen::Vector3d velBtoP(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velBtoP,v,r);
    }
    virtual Eigen::Vector3d velPtoB(const Eigen::Vector3d &v,const Eigen::Vector3d &r) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velPtoB,v,r);
    }
    virtual Eigen::Vector3d omegaBtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaBtoI,v);
    }
    virtual Eigen::Vector3d omegaItoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaItoB,v);
    }
    virtual Eigen::Vector3d omegaBtoP(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaBtoP,v);
    }
    virtual Eigen::Vector3d omegaPtoB(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaPtoB,v);
    }
    virtual Eigen::Vector3d posI() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,posI);
    }
    virtual Eigen::Vector3d posP() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,posP);
    }
    virtual Eigen::Vector3d velI() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velI);
    }
    virtual Eigen::Vector3d velP() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,velP);
    }
    virtual Quaternion qI() const override{
        PYBIND11_OVERRIDE(Quaternion,Base,qI);
    }
    virtual Quaternion qP() const override{
        PYBIND11_OVERRIDE(Quaternion,Base,qP);
    }
    virtual Eigen::Vector3d omegaI() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaI);
    }
    virtual Eigen::Vector3d omegaP() const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,omegaP);
    }
};

DECLARE_CLASS_WITHOUT_TRAMPOLINE(PhysicalAssetAccessor,AssetAccessor)
    friend class SimulationManager;
    public:
    PhysicalAssetAccessor(std::shared_ptr<PhysicalAsset> a);
    virtual ~PhysicalAssetAccessor();
    template<class T>
    bool isinstance(){
        return util::isinstance<T>(asset);
    }
    protected:
    std::weak_ptr<PhysicalAsset> asset;
};

void exportPhysicalAsset(py::module &m);