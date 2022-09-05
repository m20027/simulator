#pragma once
#include <functional>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/CXX11/Tensor>
#include "MathUtility.h"
#include "Fighter.h"
#include "Utility.h"
namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(Propulsion,PhysicalAsset)
	/*
	F-16 engine model in [1]. Supersonic max thrust value is obtained from a figure in [2].
	References:
	[1] Stevens, Brian L., et al. "Aircraft control and simulation: dynamics, controls design, and autonomous systems."" John Wiley & Sons, 2015.
    [2] Henrdrick, P., et al. "A Flight Thrust Deck for the F100 Turbofan of the F-16 Aircraft." 26th Congress of International Council of the Aeronautical Sciences, 2008.
	*/
    public:
    Eigen::Tensor<double,2,Eigen::RowMajor> tminTable,tmilTable,tmaxTable;
    Eigen::VectorXd alts,machs,machsEx;
    std::vector<Eigen::VectorXd> points,pointsEx;
    Propulsion(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    double calcThrust(double pCmd,double alt,double rmach);
    double trimPower(double tgtThrust,double alt,double Mach);
};
DECLARE_CLASS_WITH_TRAMPOLINE(CoordinatedFighter,Fighter)
	/*
	Point mass approximation assuming "coordinated" flight.
	Aerodynamic coefficients and engine characteristics are based on public released F-16 information listed below.
    Most of the model is based on [1], wave drag is based on [3], and supersonic CD0 is based on [4].
	References:
	[1] Stevens, Brian L., et al. "Aircraft control and simulation: dynamics, controls design, and autonomous systems." John Wiley & Sons, 2015.
	[3] Krus, P., et al. "Modelling of Transonic and Supersonic Aerodynamics for Conceptual Design and Flight Simulation." Proceedings of the 10th Aerospace Technology Congress, 2019.
    [4] Webb, T. S., et al. "Correlation of F-16 Aerodynamics and Performance Predictions with Early Flight Test Results." Agard Conference Proceedings, N242, 1977.
	*/
    //protected:
    public:
    //model parameters
    double m,S,rollMax,sideGLimit;
    std::shared_ptr<Propulsion> engine;
    Eigen::Tensor<double,2,Eigen::RowMajor> cxTable,cmTable;
    Eigen::Tensor<double,1,Eigen::RowMajor> czTable,trimmedDe;
    Eigen::VectorXd aoaTable,deTable,cdwTable;
    double minAoa,maxAoa,maxCz,minCz;
    public:
    //constructors & destructor
    CoordinatedFighter(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~CoordinatedFighter();
    //functions
    virtual void makeChildren();
    virtual double trimAoa(double tgtCz);
    virtual double cm(double aoa_,double de_);
    virtual double cmtrim(double aoa_);
    virtual double cx(double aoa_);
    virtual double cz(double aoa_);
    virtual double cxa(double aoa_);
    virtual double cza(double aoa_);
    virtual double cl(double aoa_);
    virtual double cd(double aoa_,double mach_);
    virtual double cdw(double mach_);
    virtual Eigen::Vector3d calcAero(double aoa_,double mach_);
    virtual void calcMotion(double dt) override;
    DECLARE_CLASS_WITHOUT_TRAMPOLINE(FlightController,Fighter::FlightController)
        public:
        double lambdaVel,lambdaTheta,clampTheta;
        double kVel,kAccel,kPower,kTheta,kOmega,kAoa,kEps;
        double pitchLimit,pitchLimitThreshold;
        //constructors & destructor
        FlightController(const nl::json& modelConfig_,const nl::json& instanceConfig_);
        virtual nl::json getDefaultCommand() override;
        virtual nl::json calc(const nl::json &cmd) override;
        nl::json calcDirect(const nl::json &cmd);
        nl::json calcFromDirAndVel(const nl::json &cmd);
    };
};

DECLARE_TRAMPOLINE(CoordinatedFighter)
    virtual double trimAoa(double tgtCz) override{
        PYBIND11_OVERRIDE(double,Base,trimAoa,tgtCz);
    }
    virtual double cm(double aoa_,double de_) override{
        PYBIND11_OVERRIDE(double,Base,cm,aoa_,de_);
    }
    virtual double cmtrim(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cmtrim,aoa_);
    }
    virtual double cx(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cx,aoa_);
    }
    virtual double cz(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cz,aoa_);
    }
    virtual double cxa(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cxa,aoa_);
    }
    virtual double cza(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cza,aoa_);
    }
    virtual double cl(double aoa_) override{
        PYBIND11_OVERRIDE(double,Base,cl,aoa_);
    }
    virtual double cd(double aoa_,double mach_) override{
        PYBIND11_OVERRIDE(double,Base,cd,aoa_,mach_);
    }
    virtual double cdw(double mach_) override{
        PYBIND11_OVERRIDE(double,Base,cdw,mach_);
    }
    virtual Eigen::Vector3d calcAero(double aoa_,double mach_) override{
        PYBIND11_OVERRIDE(Eigen::Vector3d,Base,calcAero,aoa_,mach_);
    }
};

void exportCoordinatedFighter(py::module& m);