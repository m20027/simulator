#pragma once
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <Eigen/CXX11/Tensor>
#include "MathUtility.h"
#include "Controller.h"
#include "PhysicalAsset.h"
#include "Track.h"
namespace py=pybind11;
namespace nl=nlohmann;
class Fighter;
class MissileSensor;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(PropNav,Controller)
    public:
    double gain;
    //constructors & destructor
    //PropNav();
    PropNav(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    void control() override;
    std::pair<Eigen::Vector3d,Eigen::Vector3d> calc(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt);
};

DECLARE_CLASS_WITH_TRAMPOLINE(Missile,PhysicalAsset)
    friend void makeRangeTableSub(std::mutex& m,std::promise<Eigen::VectorXd> p,const nl::json& modelConfig_,const nl::json& instanceConfig_,const Eigen::MatrixXd& args);
    public:
    enum class Mode{
        GUIDED,
        SELF,
        MEMORY
    };
    //configで指定するもの
    double tMax,tBurn,hitD,minV;
    double m,thrust,maxLoadG;
    double Sref,maxA,maxD,l,d,ln,lcg,lw,bw,bt,thicknessRatio,Sw,St,Isp,boostMaxG,boostMaxM,boostAlt;
    //位置、姿勢等の運動状態に関する追加変数
    Eigen::Vector3d accel;
    double accelScalar;
    //その他の内部変数
    std::vector<Eigen::VectorXd> rangeTablePoints;
    Eigen::Tensor<double,6> rangeTable;
    Track3D target;
    double targetUpdatedTime;
    bool hasLaunched;
    Mode mode;
    double launchedT;
    Eigen::Vector3d estTPos,estTVel;
    //子要素
    std::weak_ptr<MissileSensor> sensor;
    public:
    //constructors & destructor
    Missile(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~Missile();
    //functions
    virtual void makeChildren();
    virtual void validate();
    virtual void readConfig();
    virtual void setDependency();
    virtual void perceive(bool inReset);
    virtual void control();
    virtual void behave();
    virtual void kill();
    virtual void calcMotion(double tAftLaunch,double dt);
    virtual bool hitCheck(const Eigen::Vector3d &tpos,const Eigen::Vector3d &tpos_prev);
    virtual void calcQ();
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt);
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa);
    virtual double calcRange(double vs,double hs,double vt,double ht,double obs,double aa);
    virtual double calcRangeSub(double vs,double hs,double vt,double ht,double obs,double aa,double r);
    virtual void makeRangeTable(const std::string& dstPath);
};

DECLARE_TRAMPOLINE(Missile)
    virtual void readConfig() override{
        PYBIND11_OVERRIDE(void,Base,readConfig);
    }
    virtual void calcMotion(double tAftLaunch,double dt) override{
        PYBIND11_OVERRIDE(void,Base,calcMotion,tAftLaunch,dt);
    }
    virtual bool hitCheck(const Eigen::Vector3d &tpos,const Eigen::Vector3d &tpos_prev) override{
        PYBIND11_OVERRIDE(bool,Base,hitCheck,tpos,tpos_prev);
    }
    virtual void calcQ() override{
        PYBIND11_OVERRIDE(void,Base,calcQ);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt);
    }
    virtual double getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
        PYBIND11_OVERRIDE(double,Base,getRmax,rs,vs,rt,vt,aa);
    }
    virtual double calcRange(double vs,double hs,double vt,double ht,double obs,double aa) override{
        PYBIND11_OVERRIDE(double,Base,calcRange,vs,hs,vt,ht,obs,aa);
    }
    virtual double calcRangeSub(double vs,double hs,double vt,double ht,double obs,double aa,double r) override{
        PYBIND11_OVERRIDE(double,Base,calcRangeSub,vs,hs,vt,ht,obs,aa,r);
    }
    virtual void makeRangeTable(const std::string& dstPath){
        PYBIND11_OVERRIDE(void,Base,makeRangeTable,dstPath);
    }
};
void makeRangeTableSub(std::mutex& m,std::promise<Eigen::VectorXd> p,const nl::json& modelConfig_,const nl::json& instanceConfig_,const Eigen::MatrixXd& args);

void exportMissile(py::module& m);

/*
Based on [Ekker 1994] Appendix. B, but modified a bit.
(1) All values should be provided in MKS units. (angles should be in rad.)
(2) Transonic region (M=0.95 to 1.2) is also calculated by same method as either sub/supersonic region to simplify, although it is not so accuarate.
*/
double areapogv(double d,double ln);
double bdycla(double d,double l,double ln,double Mach,double alpha=0.0);
double bdycma_nlpart(double d,double l,double ln,double lcg,double Mach,double alpha=0.0);
double bigkbwa(double M,double A,double r,double s);
double bigkbwna(double M,double A,double r,double s);
double bigkbwsb(double r,double s);
double bigkwb(double r,double s);
double bkbwsba(double M,double A,double r,double s);
double bkbwsbna(double M,double A,double r,double s);
double bkbwspa(double M,double A,double r,double s);
double bkbwspna(double M,double A,double r,double s);
double bsdrgcf(double Mach);
double bsdrgsp(double Mach);
double cdbbody(double d,double l,double ln,double Mach,double atr);
double cdlcomp(double bw,double Sw,double Mach);
double cdlwing(double bw,double Sw,double Mach_,double atr);
double cdobody(double d,double l,double ln,double Mach,double alt,double nf=1.1,bool pwroff=false);
double cdowbt(double d,double l,double ln,double bw,double Sw,double thickw,double bt,double St,double thickt,double Mach,double alt,double nf=1.1,bool pwroff=false);
double cdowing(double bw,double Sw,double thick,double Mach_,double alt,double nf=1.1);
double cdtrim(double d,double l,double ln,double bw,double Sw,double thickw,double bt,double St,double thickt,double Mach,double alt,double atr,double dtr,double nf=1.1,bool pwroff=false);
double cfturbfp(double chardim,double Mach,double alt);
std::pair<double,double> clacma(double d,double l,double ln,double lcg,double bw,double Sw,double lw,double bt,double St,double Mach,double alpha=0.0);
double clawsub(double M,double A,double lc2,double kappa);
double clawsup(double M,double A);
std::pair<double,double> cldcmd(double d,double l,double ln,double lcg,double bt,double St,double Mach);
double cldwbt(double d,double bt,double St,double Mach);
double cpogvemp(double d,double ln,double M);
double cpogvsb(double ln,double d);
double dedasub(double A,double b,double lc4,double lam,double lH,double hH);
double dedasup(double M,double lam,double x0);
double dragfctr(double fineness);
double k2mk1(double fineness);
double smlkbw(double r,double s);
double smlkwb(double r,double s);
double sscfdccc(double alpha,double M);
double surfogv1(double diameter,double length);
double vologv1(double logv,double dbase);
double wvdrgogv(double diameter,double length,double Mach);
double xcrbwabh(double M,double r,double cr);
double xcrbwabl(double M,double A,double r,double s);
double xcrbwnab(double M,double r,double cr);
double xcrbwsub(double M,double A,double r,double s);
double xcrw(double M,double A);
double xcrwba(double r,double s);
double xcrwbd(double r,double s);
double xcrwbsub(double M,double A);