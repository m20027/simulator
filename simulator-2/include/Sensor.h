#pragma once
#include <memory>
#include <vector>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include "MathUtility.h"
#include "PhysicalAsset.h"
#include "Track.h"
namespace py=pybind11;
namespace nl=nlohmann;
class Fighter;
class Missile;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(Sensor,PhysicalAsset)
    public:
    //constructors & destructor
    Sensor(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Sensor();
    //functions
    virtual void setDependency();
};
DECLARE_CLASS_WITH_TRAMPOLINE(Sensor3D,Sensor)
    public:
    //constructors & destructor
    Sensor3D(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Sensor3D();
    //functions
    virtual std::pair<bool,std::vector<Track3D>> isTrackingAny();
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track3D> isTracking(const Track3D& target_);
    virtual std::pair<bool,Track3D> isTracking(const boost::uuids::uuid& target_);
};
DECLARE_TRAMPOLINE(Sensor3D)
    virtual std::pair<bool,std::vector<Track3D>> isTrackingAny() override{
        typedef  std::pair<bool,std::vector<Track3D>> retType;
        PYBIND11_OVERRIDE(retType,Base,isTrackingAny);
    }
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_) override{
        typedef  std::pair<bool,Track3D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual std::pair<bool,Track3D> isTracking(const Track3D& target_) override{
        typedef  std::pair<bool,Track3D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual std::pair<bool,Track3D> isTracking(const boost::uuids::uuid& target_) override{
        typedef  std::pair<bool,Track3D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
};
DECLARE_CLASS_WITH_TRAMPOLINE(Sensor2D,Sensor)
    public:
    //constructors & destructor
    Sensor2D(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Sensor2D();
    //functions
    virtual std::pair<bool,std::vector<Track2D>> isTrackingAny();
    virtual std::pair<bool,Track2D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track2D> isTracking(const Track2D& target_);
    virtual std::pair<bool,Track2D> isTracking(const boost::uuids::uuid& target_);
};
DECLARE_TRAMPOLINE(Sensor2D)
    virtual std::pair<bool,std::vector<Track2D>> isTrackingAny() override{
        typedef  std::pair<bool,std::vector<Track2D>> retType;
        PYBIND11_OVERRIDE(retType,Base,isTrackingAny);
    }
    virtual std::pair<bool,Track2D> isTracking(std::weak_ptr<PhysicalAsset> target_) override{
        typedef  std::pair<bool,Track2D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual std::pair<bool,Track2D> isTracking(const Track2D& target_) override{
        typedef  std::pair<bool,Track2D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
    virtual std::pair<bool,Track2D> isTracking(const boost::uuids::uuid& target_) override{
        typedef  std::pair<bool,Track2D> retType;
        PYBIND11_OVERRIDE(retType,Base,isTracking,target_);
    }
};


DECLARE_CLASS_WITHOUT_TRAMPOLINE(AircraftRadar,Sensor3D)
    public:
    //parameters
    double Lref,thetaFOR;
    //internal variables
    std::vector<Track3D> track;
    public:
    //constructors & destructor
    AircraftRadar(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~AircraftRadar();
    //functions
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual std::pair<bool,std::vector<Track3D>> isTrackingAny();
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track3D> isTracking(const Track3D& target_);
    virtual std::pair<bool,Track3D> isTracking(const boost::uuids::uuid& target_);
};

DECLARE_CLASS_WITHOUT_TRAMPOLINE(MWS,Sensor2D)
    public:
    //parameters
    //internal variables
    std::vector<Track2D> track;
    public:
    //constructors & destructor
    MWS(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~MWS();
    //functions
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual std::pair<bool,std::vector<Track2D>> isTrackingAny();
    virtual std::pair<bool,Track2D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track2D> isTracking(const Track2D& target_);
    virtual std::pair<bool,Track2D> isTracking(const boost::uuids::uuid& target_);
};

DECLARE_CLASS_WITHOUT_TRAMPOLINE(MissileSensor,Sensor3D)
    public:
    //parameters
    double Lref,thetaFOR,thetaFOV;
    //internal variables
    std::vector<Track3D> track;
    Track3D target;
    bool isActive;
    Eigen::Vector3d estTPos;
    public:
    //constructors & destructor
    MissileSensor(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~MissileSensor();
    //functions
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void kill();
    virtual std::pair<bool,std::vector<Track3D>> isTrackingAny();
    virtual std::pair<bool,Track3D> isTracking(std::weak_ptr<PhysicalAsset> target_);
    virtual std::pair<bool,Track3D> isTracking(const Track3D& target_);
    virtual std::pair<bool,Track3D> isTracking(const boost::uuids::uuid& target_);
};

void exportSensor(py::module& m);