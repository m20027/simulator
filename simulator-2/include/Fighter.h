#pragma once
#include <deque>
#include <vector>
#include <functional>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include "MathUtility.h"
#include "PhysicalAsset.h"
#include "Controller.h"
#include "Track.h"
namespace py=pybind11;
namespace nl=nlohmann;
class Missile;
class AircraftRadar;
class MWS;
class CommunicationBuffer;

DECLARE_CLASS_WITH_TRAMPOLINE(Fighter,PhysicalAsset)
    public:
    double rcsScale;//as a dimensionless value
    std::weak_ptr<AircraftRadar> radar;
    std::weak_ptr<MWS> mws;
    std::vector<std::weak_ptr<Missile>> missiles;
    std::weak_ptr<Missile> dummyMissile;
    std::vector<std::pair<Track3D,bool>> missileTargets;
    int nextMsl,numMsls,remMsls;
    bool isDatalinkEnabled;
    std::vector<Track3D> track;
    std::vector<std::vector<std::string>> trackSource;
    std::string datalinkName;
    bool launchable;
    Track3D target;int targetID;
    //parameters
    public:
    //constructors & destructor
    Fighter(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Fighter();
    //functions
    virtual void makeChildren();
    virtual void validate();
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual void kill();
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track3D> isTracking(const Track3D& target_);
    virtual std::pair<bool,Track3D> isTracking(const boost::uuids::uuid& target_);
    virtual void setFlightControllerMode(const std::string& ctrlName);
    virtual void calcMotion(double dt)=0;
    virtual Eigen::Vector3d toEulerAngle();
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt);
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa);
    //座標変換
    virtual Eigen::Vector3d relHtoI(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d relItoH(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d absHtoI(const Eigen::Vector3d &v) const;
    virtual Eigen::Vector3d absItoH(const Eigen::Vector3d &v) const;
    virtual std::shared_ptr<AssetAccessor> getAccessor();
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(SensorDataSharer,Controller)
        public:
        SensorDataSharer(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void perceive(bool inReset) override;
    };
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(SensorDataSanitizer,Controller)
        public:
        std::map<std::string,double> lastSharedTime;
        SensorDataSanitizer(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void perceive(bool inReset) override;
    };
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(OtherDataSharer,Controller)
        public:
        OtherDataSharer(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void perceive(bool inReset) override;
        void control() override;
    };
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(OtherDataSanitizer,Controller)
        public:
        std::map<std::string,double> lastSharedTimeOfAgentObservable;
        std::map<std::string,double> lastSharedTimeOfFighterObservable;
        double lastSharedTimeOfAgentCommand;
        OtherDataSanitizer(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void perceive(bool inReset) override;
        void control() override;
    };
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(HumanIntervention,Controller)
        //delay for shot command in order to simulate approval by human operator.
        public:
        int capacity;
        double delay,cooldown;//in seconds.
        std::deque<std::pair<double,nl::json>> recognizedShotCommands;
        HumanIntervention(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void control() override;
    };
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(WeaponController,Controller)
        public:
        WeaponController(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        void control() override;
    };
    DECLARE_CLASS_WITH_TRAMPOLINE(FlightController,Controller)
        public:
        //constructors & destructor
        FlightController(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        virtual ~FlightController();
        virtual void control() override;
        virtual nl::json getDefaultCommand();
        virtual nl::json calc(const nl::json &cmd);
        virtual void setMode(const std::string& mode_);
        protected:
        std::string mode;
    };
};
    DECLARE_TRAMPOLINE(Fighter::FlightController)
        virtual void control() override{
            PYBIND11_OVERRIDE(void,Base,control);
        }
        virtual nl::json getDefaultCommand() override{
            PYBIND11_OVERRIDE(nl::json,Base,getDefaultCommand);
        }
        virtual nl::json calc(const nl::json &cmd) override{
            PYBIND11_OVERRIDE(nl::json,Base,calc,cmd);
        }
        virtual void setMode(const std::string& mode_) override{
            PYBIND11_OVERRIDE(void,Base,setMode,mode_);
        }
    };

DECLARE_TRAMPOLINE(Fighter)
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_) override{
        typedef std::pair<bool,Track3D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual std::pair<bool,Track3D> isTracking(const Track3D&  target_) override{
        typedef std::pair<bool,Track3D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual void setFlightControllerMode(const std::string& ctrlName) override{
        PYBIND11_OVERRIDE(void,Base,setFlightControllerMode,ctrlName);
    }
    virtual void calcMotion(double dt) override{
        PYBIND11_OVERRIDE_PURE(void,Base,calcMotion,dt);
    }
    virtual Eigen::Vector3d toEulerAngle() override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,toEulerAngle);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt,aa);
    }
    virtual Eigen::Vector3d relHtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relHtoI,v);
    }
    virtual Eigen::Vector3d relItoH(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,relItoH,v);
    }
    virtual Eigen::Vector3d absHtoI(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absHtoI,v);
    }
    virtual Eigen::Vector3d absItoH(const Eigen::Vector3d &v) const override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,absItoH,v);
    }
};

DECLARE_CLASS_WITH_TRAMPOLINE(FighterAccessor,PhysicalAssetAccessor)
    friend class SimulationManager;
    public:
    FighterAccessor(std::shared_ptr<Fighter> a);
    virtual ~FighterAccessor();
    template<class T>
    bool isinstance(){
        return util::isinstance<T>(asset);
    }
    virtual void setFlightControllerMode(const std::string& ctrlName="");
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa);
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt);
    protected:
    std::weak_ptr<Fighter> asset;
};
DECLARE_TRAMPOLINE(FighterAccessor)
    virtual void setFlightControllerMode(const std::string& ctrlName="") override{
        PYBIND11_OVERRIDE(void,Base,setFlightControllerMode,ctrlName);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa) override{
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt,aa);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt) override{
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt);
    }
};

void exportFighter(py::module& m);