#pragma once
#include <deque>
#include <vector>
#include <functional>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include "MathUtility.h"
#include "Fighter.h"
#include "Controller.h"
#include "Track.h"
namespace py=pybind11;
namespace nl=nlohmann;
class Missile;
class FighterSensor;
class CommunicationBuffer;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(MassPointFighter,Fighter)
    public:
    //model parameters
    double vMin,vMax,aMin,aMax,rollMax,pitchMax,yawMax;
    public:
    //constructors & destructor
    MassPointFighter(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~MassPointFighter();
    //functions
    virtual void calcMotion(double dt) override;
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(FlightController,Fighter::FlightController)
        public:
        //constructors & destructor
        FlightController(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        virtual ~FlightController();
        virtual nl::json getDefaultCommand() override;
        virtual nl::json calc(const nl::json &cmd) override;
    };
};

void exportMassPointFighter(py::module& m);