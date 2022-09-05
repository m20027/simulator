#include "Missile.h"
#include <cmath>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <boost/math/special_functions/ellint_2.hpp>
#include <boost/math/tools/roots.hpp>
#include "MathUtility.h"
#include "Units.h"
#include "SimulationManager.h"
#include "Agent.h"
#include "Fighter.h"
#include "Sensor.h"
#include "CommunicationBuffer.h"
using namespace util;
PropNav::PropNav(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    manager=instanceConfig.at("manager");
    try{
        parent=Entity::from_json_weakref<Asset>(instanceConfig.at("parent"));
    }catch(...){
        //
    }
    //modelConfigで指定するもの
    gain=getValueFromJsonKR(modelConfig,"G",randomGen);
    commands={
        {"accel",Eigen::Vector3d(0,0,0)},
        {"omega",Eigen::Vector3d(0,0,0)}
    };
}
void PropNav::control(){
    auto msl=getShared<Missile>(parent);
    if(msl->hasLaunched && msl->isAlive()){
        nl::json pc=parent.lock()->commands.at("Navigator");
        auto ret=calc(pc.at("rs"),pc.at("vs"),pc.at("rt"),pc.at("vt"));
        commands={
            {"accel",ret.first},
            {"omega",ret.second}
        };
    }
}
std::pair<Eigen::Vector3d,Eigen::Vector3d> PropNav::calc(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    Eigen::Vector3d rr=rt-rs;
    Eigen::Vector3d vr=vt-vs;
    double R2=rr.squaredNorm();
    if(R2==0){
        //本来は1/|rr|のスケールで発散するが、rrの向きが不定となる|rr|=0のときに限り、rrとvsが平行とみなすことによって指令値を0とする。
        return std::make_pair(Eigen::Vector3d(0,0,0),Eigen::Vector3d(0,0,0));
    }
    double Vs2=vs.squaredNorm();
    if(Vs2==0){
        //vsの向きが不定となる|vs|=0のときに限り、vsとomegaが平行とみなすことによって指令値を0とする。
        //ただし、現状のMissileクラスの運動モデルの実装の方が|v|=0に対応していないためこのような状況下では使用されない。
        return std::make_pair(Eigen::Vector3d(0,0,0),Eigen::Vector3d(0,0,0));
    }
    Eigen::Vector3d omega=rr.cross(vr)/R2;
    Eigen::Vector3d accel=gain*omega.cross(vs);
    return std::make_pair(accel,vs.cross(accel)/Vs2);
}

Missile::Missile(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:PhysicalAsset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    readConfig();
    //位置、姿勢等の運動状態に関する変数の初期化
    if(!parent.expired()){
        auto p=parent.lock();
        motion.pos=p->posI();
        motion.vel=p->velI();
        motion.omega=p->omegaI();
        motion.q=p->qI();
        vel_prev=p->vel_prev;
        pos_prev=p->pos_prev;
    }else{
        motion.pos<<0,0,0;
        motion.vel<<0,0,0;
        motion.omega<<0,0,0;
        motion.q=Quaternion(1,0,0,0);
        vel_prev=motion.vel;
        if(!manager->expired()){
            pos_prev=motion.pos-motion.vel*manager->getBaseTimeStep();
        }else{
            pos_prev=motion.pos;
        }
    }
    accel<<0,0,0;
    accelScalar=0.0;
    //その他の内部変数
    target=Track3D();
    hasLaunched=false;
    isAlive_=true;
    mode=Mode::MEMORY;
    estTPos=estTVel=Eigen::Vector3d(0,0,0);
    launchedT=0.0;
    observables["hasLaunched"]=hasLaunched;
    observables["mode"]=enumToJson(mode);
    observables["target"]=Track3D(target);
}
Missile::~Missile(){}
void Missile::makeChildren(){
    std::string snsrName=fullName+":Sensor";
    nl::json sub={
        {"fullName",fullName+":Sensor"},
        {"seed",randomGen()},
        {"parent",this->weak_from_this()},
        {"isBound",true}
    };
    sensor=manager->generateAsset<MissileSensor>(
        "PhysicalAsset",
        modelConfig.at("Sensor"),
        sub
    );
    sub={
        {"fullName",fullName+":Navigator"},
        {"parent",this->weak_from_this()}
    };
    controllers["Navigator"]=manager->generateAsset<Controller>("Controller",modelConfig.at("Navigator"),sub);
}
void Missile::validate(){
    auto os=py::module_::import("os");
    auto self=py::module_::import("ASRCAISim1.libCore");
    std::string fileName=modelConfig["rangeTable"];
    std::string filePath;
    auto np=py::module_::import("numpy");
    filePath=py::cast<std::string>(os.attr("path").attr("join")(os.attr("getcwd")(),fileName));
    py::object loaded;
    if(py::cast<bool>(os.attr("path").attr("exists")(filePath))){
        loaded=np.attr("load")(filePath);
    }else{
        filePath=py::cast<std::string>(os.attr("path").attr("join")(
            os.attr("path").attr("dirname")(self.attr("__file__")),fileName));
        try{
            loaded=np.attr("load")(filePath);
        }catch(...){
            std::cout<<fileName<<" is not found."<<std::endl;
            filePath=py::cast<std::string>(os.attr("path").attr("join")(os.attr("getcwd")(),fileName));
            makeRangeTable(filePath);
            loaded=np.attr("load")(filePath);
        }
    }
    rangeTablePoints={
        py::cast<Eigen::VectorXd>(loaded["vs"]),
        py::cast<Eigen::VectorXd>(loaded["hs"]),
        py::cast<Eigen::VectorXd>(loaded["vt"]),
        py::cast<Eigen::VectorXd>(loaded["ht"]),
        py::cast<Eigen::VectorXd>(loaded["obs"]),
        py::cast<Eigen::VectorXd>(loaded["aa"])
    };
    rangeTable=py::cast<Eigen::Tensor<double,6>>(loaded["ranges"]);
    auto p=parent.lock();
    motion.pos=p->posI();
    motion.vel=p->velI();
    motion.omega=p->omegaI();
    motion.q=p->qI();
    vel_prev=p->vel_prev;
    pos_prev=p->pos_prev;
}
void Missile::readConfig(){
    tMax=getValueFromJsonKR(modelConfig,"tMax",randomGen);
    hitD=getValueFromJsonKR(modelConfig,"hitD",randomGen);
    minV=getValueFromJsonKR(modelConfig,"minV",randomGen);
    m=getValueFromJsonKR(modelConfig,"mass",randomGen);
    maxLoadG=getValueFromJsonKR(modelConfig,"maxLoadG",randomGen);
    maxA=deg2rad(getValueFromJsonKR(modelConfig,"maxA",randomGen));
    maxD=deg2rad(getValueFromJsonKR(modelConfig,"maxD",randomGen));
    l=getValueFromJsonKR(modelConfig,"length",randomGen);
    d=getValueFromJsonKR(modelConfig,"diameter",randomGen);
    ln=getValueFromJsonKR(modelConfig,"lengthN",randomGen);
    lcg=getValueFromJsonKR(modelConfig,"lcg",randomGen);
    lw=getValueFromJsonKR(modelConfig,"locationW",randomGen);
    bw=getValueFromJsonKR(modelConfig,"spanW",randomGen);
    bt=getValueFromJsonKR(modelConfig,"spanT",randomGen);
    thicknessRatio=getValueFromJsonKR(modelConfig,"thicknessRatio",randomGen);
    Sw=getValueFromJsonKR(modelConfig,"areaW",randomGen);
    St=getValueFromJsonKR(modelConfig,"areaT",randomGen);
    Isp=getValueFromJsonKR(modelConfig,"Isp",randomGen);
    boostMaxG=getValueFromJsonKR(modelConfig,"boostMaxG",randomGen);
    boostMaxM=getValueFromJsonKR(modelConfig,"boostMaxM",randomGen);
    boostAlt=getValueFromJsonKR(modelConfig,"boostAlt",randomGen);
    Sref=M_PI*d*d/4;
    std::vector<double> atm=atmosphere(boostAlt);
    double T=atm[0];
    double a=atm[1];
    double P=atm[2];
    double rho=atm[3];
    double dRhodH=atm[4];
    double nu=atm[5];
    double dVb=boostMaxM*a;
    double Wp=m*(1.0-exp(-dVb/gravity/Isp));
    tBurn=dVb/(boostMaxG*gravity);
    thrust=Isp*Wp*gravity/tBurn;
}
void Missile::setDependency(){
    dependencyChecker->addDependency(SimPhase::PERCEIVE,sensor.lock());
    controllers["Navigator"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,getShared<Asset>(this->shared_from_this()));
    for(auto&& asset:manager->getAssets([](std::shared_ptr<const Asset> asset)->bool{
            return isinstance<Fighter>(asset);
        })){
        dependencyChecker->addDependency(SimPhase::BEHAVE,getShared(asset));
    }
}
void Missile::perceive(bool inReset){
    PhysicalAsset::perceive(inReset);
    observables["hasLaunched"]=hasLaunched;
    observables["launchedT"]=hasLaunched ? launchedT : -1;
    if(hasLaunched){
        std::pair<bool,Track3D> ret;
        if(sensor.lock()->isActive){
            ret=sensor.lock()->isTracking(target);
        }else{
            ret=std::make_pair(false,Track3D());
        }
        if(ret.first){
            mode=Mode::SELF;
            target=ret.second.copy();
            estTPos=target.posI();
            estTVel=target.velI();
        }else{
            auto data=communicationBuffers["MissileComm:"+getFullName()].lock()->receive("target");
            ret=std::dynamic_pointer_cast<Fighter>(parent.lock())->isTracking(target);
            if(data.first>=0 && data.first>=targetUpdatedTime && !data.second.get<Track3D>().is_none()){
                mode=Mode::GUIDED;
                target=data.second;
                targetUpdatedTime=manager->getTime();
                estTPos=target.posI();
                estTVel=target.velI();
            }else{
                mode=Mode::MEMORY;
                estTPos+=estTVel*manager->getBaseTimeStep();
            }
        }
        observables["mode"]=enumToJson(mode);
        observables["target"]=Track3D(target);
    }
}
void Missile::control(){
    auto launchFlag=communicationBuffers["MissileComm:"+getFullName()].lock()->receive("launch");
    if(!hasLaunched && (launchFlag.first>=0 && launchFlag.second)){
        target=communicationBuffers["MissileComm:"+getFullName()].lock()->receive("target").second;
        targetUpdatedTime=manager->getTime();
        launchedT=manager->getTime();
        calcQ();
        mode=Mode::GUIDED;
        estTPos=target.posI();
        estTVel=target.velI();
        hasLaunched=true;
    }
    if(hasLaunched){
        commands={
            {"Sensor",nl::json::array({
                {
                    {"name","steering"},
                    {"estTPos",estTPos},
                    {"estTVel",estTVel}
                }
            })},
            {"Navigator",{
                {"rs",posI()},
                {"vs",velI()},
                {"rt",estTPos},
                {"vt",estTVel}
            }}
        };
        double L=(estTPos-posI()).norm();
        if(L<sensor.lock()->Lref){
            commands["Sensor"].push_back({
                {"name","activate"},
                {"target",target}
            });
        }
    }
}
void Missile::behave(){
    if(hasLaunched){
        double tAftLaunch=manager->getTime()-launchedT;
        calcMotion(tAftLaunch,manager->getBaseTimeStep());
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->getTeam()!=getTeam() && isinstance<Fighter>(asset);
        })){
            Eigen::Vector3d tpos=e.lock()->posI();
            Eigen::Vector3d tpos_prev=e.lock()->pos_prev;
            if(
                e.lock()->isAlive() &&
                hitCheck(tpos,tpos_prev)
            ){//命中
                e.lock()->kill();
                nl::json cmd={
                    {"wpn",this->shared_from_this()},
                    {"tgt",e.lock()}
                };
                manager->triggerEvent("Hit",cmd);
                this->kill();
            }
        }
        if(
            tAftLaunch>=tMax ||
            motion.pos(2)>0 ||
            (tAftLaunch>=tBurn && motion.vel.norm()<minV)
        ){//最大飛翔時間超過or地面に激突or燃焼後に速度が規定値未満
            this->kill();
        }
    }else{
        std::shared_ptr<PhysicalAsset> parent_=parent.lock();
        motion.pos=parent_->posI();
        motion.vel=parent_->velI();
        motion.omega=parent_->omegaI();
        motion.q=parent_->qI();
        pos_prev=parent_->pos_prev;
        vel_prev=parent_->vel_prev;
    }
}
void Missile::kill(){
    sensor.lock()->kill();
    for(auto&& e:controllers){
        e.second.lock()->kill();
    }
    this->PhysicalAsset::kill();
}
void Missile::calcMotion(double tAftLaunch,double dt){
    pos_prev=motion.pos;
    vel_prev=motion.vel;
    double V=motion.vel.norm();
    auto propNav=getShared<PropNav>(controllers["Navigator"]);
    Eigen::Vector3d accelCmd=propNav->commands["accel"];
    Eigen::Vector3d omegaV=propNav->commands["omega"];
    double omega=omegaV.norm();
    double omegaMax=maxLoadG*gravity/V;
    if(omega>omegaMax){
        omegaV*=(omegaMax/omega);
        omega=omegaMax;
        accelCmd=omegaV.cross(motion.vel);
    }
    double alt=-motion.pos(2);
    double nf=1.1;
    bool pwroff=tAftLaunch>=tBurn;
    double thrust_=pwroff ? 0.0 : thrust;
    std::vector<double> atm=atmosphere(alt);
    double T=atm[0];
    double a=atm[1];
    double P=atm[2];
    double rho=atm[3];
    double dRhodH=atm[4];
    double nu=atm[5];
    double M=V/a;
    double qd=0.5*rho*V*V;
    Eigen::Vector3d ex=motion.vel/V;
    Eigen::Vector3d ey=(accelCmd-Eigen::Vector3d(0,0,gravity)).cross(ex)*m;
    double desiredSideForce=ey.norm();
    Eigen::Vector3d sideAx;
    double aoa,CL,CD;
    if(desiredSideForce>0){
        ey/=desiredSideForce;
        sideAx=ex.cross(ey);
        std::pair<double,double> tmp=cldcmd(d,l,ln,lcg,bt,St,M);
        double cld=tmp.first;
        double cmd=tmp.second;
        tmp=clacma(d,l,ln,lcg,bw,Sw,lw,bt,St,M,0.0);
        double cla0=tmp.first;
        double cma0=tmp.second;
        auto cm=[&](double aoa_){
            return (cma0+bdycma_nlpart(d,l,ln,lcg,M,aoa_))*aoa_+cmd*maxD;
        };
        double aoaLimit=maxA;
        if(cm(maxA)<=0){
            aoaLimit=maxA;
        }else{
            try{
                boost::math::tools::eps_tolerance<double> tol(16);
                boost::uintmax_t max_iter=10;
                auto result=boost::math::tools::toms748_solve(cm,0.0,maxA,tol,max_iter);
                aoaLimit=(result.first+result.second)/2;
            }catch(std::exception& e){
                std::cout<<"exception at Missile::calcMotion (while calculating aoaLimit)"<<std::endl;
                DEBUG_PRINT_EXPRESSION(cm(0.0))
                DEBUG_PRINT_EXPRESSION(cm(maxA))
                DEBUG_PRINT_EXPRESSION(alt)
                DEBUG_PRINT_EXPRESSION(M)
                throw e;
            }
        }
        auto func=[&](double aoa_){
            std::pair<double,double> tmp=clacma(d,l,ln,lcg,bw,Sw,lw,bt,St,M,aoa_);
            double cla=tmp.first;
            double cma=tmp.second;
            return (cla-cld*cma/cmd)*(qd*Sref)*aoa_+thrust_*sin(aoa_)-desiredSideForce;
        };
        if(func(0.0)>=0){
            aoa=0.0;
        }else if(func(aoaLimit)<=0){
            aoa=aoaLimit;
        }else{
            try{
                boost::math::tools::eps_tolerance<double> tol(16);
                boost::uintmax_t max_iter=10;
                auto result=boost::math::tools::toms748_solve(func,0.0,aoaLimit,tol,max_iter);
                aoa=(result.first+result.second)/2;
            }catch(std::exception& e){
                std::cout<<"exception at Missile::calcMotion"<<std::endl;
                DEBUG_PRINT_EXPRESSION(func(0.0))
                DEBUG_PRINT_EXPRESSION(func(maxA))
                DEBUG_PRINT_EXPRESSION(alt)
                DEBUG_PRINT_EXPRESSION(M)
                DEBUG_PRINT_EXPRESSION(desiredSideForce)
                DEBUG_PRINT_EXPRESSION(thrust_)
                throw e;
            }
        }
        tmp=clacma(d,l,ln,lcg,bw,Sw,lw,bt,St,M,aoa);
        double cla=tmp.first;
        double cma=tmp.second;
        double delta=-cma/cmd*aoa;
        CL=(cla-cld*cma/cmd)*aoa;
        CD=cdtrim(d,l,ln,bw,Sw,thicknessRatio,bt,St,thicknessRatio,M,alt,aoa,delta,nf,pwroff);
    }else{
        sideAx<<0,0,0;
        CL=0.0;
        CD=cdtrim(d,l,ln,bw,Sw,thicknessRatio,bt,St,thicknessRatio,M,alt,0,0,nf,pwroff);
        aoa=0.0;
    }
    Eigen::Vector3d g(0,0,gravity);
    Eigen::Vector3d sideAccel=sideAx*(CL*qd*Sref+thrust_*sin(aoa))/m+(g-ex*(g.dot(ex)));
	//attitude and deflection are assumed to be immidiately adjustable as desired.
	accelScalar=(-CD*Sref*qd+thrust_*cos(aoa))/m+g.dot(ex);
	Eigen::Vector3d accel=ex*accelScalar;
    omegaV=motion.vel.cross(sideAccel)/(V*V);
    omega=omegaV.norm();
    if(omega*dt<1e-8){//ほぼ無回転
        motion.vel+=accel*dt;
        motion.pos+=motion.vel*dt+accel*dt*dt/2.;
    }else{
        Eigen::Vector3d ex0=motion.vel/V;
        Eigen::Vector3d ey0=omegaV.cross(ex0).normalized();
        double wt=omega*dt;
        double acc=accelScalar;
        Eigen::Vector3d vn=(ex0*cos(wt)+ey0*sin(wt)).normalized();
        motion.vel=(V+acc*dt)*vn;
        Eigen::Vector3d dr=V*dt*(ex0*sinc(wt)+ey0*oneMinusCos_x2(wt)*wt)+acc*dt*dt*(ex0*(sinc(wt)-oneMinusCos_x2(wt))+ey0*wt*(sincMinusOne_x2(wt)+oneMinusCos_x2(wt)));
        motion.pos+=dr;
        accel=(motion.vel-vel_prev)/dt;
    }
    motion.omega=omegaV;
    motion.time=manager->getTime()+dt;
    calcQ();
}
bool Missile::hitCheck(const Eigen::Vector3d &tpos,const Eigen::Vector3d &tpos_prev){
    Eigen::Vector3d r1=pos_prev;
    Eigen::Vector3d r2=tpos_prev;
    Eigen::Vector3d d1=posI()-r1;
    Eigen::Vector3d d2=tpos-r2;
    Eigen::Vector3d A=r1-r2;double a=A.norm();
    Eigen::Vector3d B=d1-d2;double b=B.norm();
    if(a<hitD){//初期位置であたり
        return true;
    }else if(b<1e-8){//相対速度がほぼ0}
        return a<hitD;
    }
    double tMin=std::min(1.,std::max(0.,-2*A.dot(B)/(b*b)));
    return (A+B*tMin).norm()<hitD;
}
void Missile::calcQ(){
    Eigen::Vector3d ex=velI().normalized();
    Eigen::Vector3d ez=Eigen::Vector3d(0,0,1);
    Eigen::Vector3d ey=ez.cross(ex);
    double Y=ey.norm();
    if(Y<1e-8){
        ey=ex.cross(Eigen::Vector3d(1,0,0)).normalized();
    }else{
        ey/=Y;
    }
    ez=ex.cross(ey);
    motion.q=Quaternion::fromBasis(ex,ey,ez);
}
double Missile::calcRange(double vs,double hs,double vt,double ht,double obs,double aa){
    double r0=30000.0;
    double r1=300000.0;
    while(!calcRangeSub(vs,hs,vt,ht,obs,aa,r0)){
        r1=r0;
        r0*=0.5;
        if(r0<hitD){
            return hitD;
        }
    }
    while(calcRangeSub(vs,hs,vt,ht,obs,aa,r1)){
        r0=r1;
        r1*=2;
    }
    double rm;
    while((r1-r0)>1.0){
        rm=(r0+r1)*0.5;
        if(calcRangeSub(vs,hs,vt,ht,obs,aa,rm)){
            r0=rm;
        }else{
            r1=rm;
        }
    }
    return (r0+r1)*0.5;
}
double Missile::calcRangeSub(double vs,double hs,double vt,double ht,double obs,double aa,double r){
    Eigen::Vector3d posBef=motion.pos;
    Eigen::Vector3d velBef=motion.vel;
    Eigen::Vector3d omegaBef=motion.omega;
    Eigen::Vector3d pos_prevBef=pos_prev;
    Eigen::Vector3d vel_prevBef=vel_prev;
    double dt=1.0/10.0;
    motion.pos=Eigen::Vector3d(-r,0,-hs);
    motion.vel=Eigen::Vector3d(cos(obs),-sin(obs),0)*std::max(vs,1e-3);
    motion.omega<<0,0,0;
    calcQ();
    Eigen::Vector3d tpos=Eigen::Vector3d(0,0,-ht);
    Eigen::Vector3d tvel=Eigen::Vector3d(-cos(aa),sin(aa),0)*vt;
    Eigen::Vector3d tpos_prev=tpos-dt*tvel;
    pos_prev=motion.pos-dt*motion.vel;
    vel_prev=motion.vel;
    double t=0;
    bool finished=false;
    bool hit=false;
    auto propNav=getShared<PropNav>(controllers["Navigator"]);
    bool hasLaunchedBef=hasLaunched;
    hasLaunched=true;
    while(!finished){
        commands={
            {"Navigator",{
                {"rs",motion.pos},
                {"vs",motion.vel},
                {"rt",tpos},
                {"vt",tvel}
            }}
        };
        propNav->control();
        calcMotion(t,dt);
        tpos_prev=tpos;
        tpos+=dt*tvel;
        t+=dt;
        if(t>=tMax || motion.pos(2)>0 ||(t>=tBurn && motion.vel.norm()<minV)){
            finished=true;
            hit=false;
        }else if(hitCheck(tpos,tpos_prev)){
            finished=true;
            hit=true;
        }
    }
    motion.pos=posBef;
    motion.vel=velBef;
    motion.omega=omegaBef;
    pos_prev=pos_prevBef;
    vel_prev=vel_prevBef;
    calcQ();
    hasLaunched=hasLaunchedBef;
    return hit;
}
double Missile::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    //When aa is omitted, current aa is used.
    double Vs=vs.norm();
    double hs=-rs(2);
    double Vt=vt.norm();
    double ht=-rt(2);
    Eigen::Vector3d dr=Eigen::Vector3d(rt(0)-rs(0),rt(1)-rs(1),0).normalized();
    Eigen::Vector3d vsh=Eigen::Vector3d(vs(0),vs(1),0).normalized();
    Eigen::Vector3d vth=Eigen::Vector3d(vt(0),vt(1),0).normalized();
    double obs=acos(std::min(1.,std::max(-1.,dr.dot(vsh))));
    double aa=acos(std::min(1.,std::max(-1.,-dr.dot(vth))));
    if(obs>1e-8){
        bool sameSide=(vsh.cross(dr)).dot(vth.cross(-dr))>=0;
        if(!sameSide){
            aa=-aa;
        }
    }
    Eigen::MatrixXd arg(1,6);
    arg<<Vs,hs,Vt,ht,obs,aa;
    return interpn<double>(rangeTablePoints,rangeTable,arg)(0);
}
double Missile::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
    double Vs=vs.norm();
    double hs=-rs(2);
    double Vt=vt.norm();
    double ht=-rt(2);
    Eigen::Vector3d dr=Eigen::Vector3d(rt(0)-rs(0),rt(1)-rs(1),0).normalized();
    Eigen::Vector3d vsh=Eigen::Vector3d(vs(0),vs(1),0).normalized();
    double obs=acos(std::min(1.,std::max(-1.,dr.dot(vsh))));
    Eigen::MatrixXd arg(1,6);
    arg<<Vs,hs,Vt,ht,obs,aa;
    return interpn<double>(rangeTablePoints,rangeTable,arg)(0);
}
void Missile::makeRangeTable(const std::string& dstPath){
    std::cout<<"makeRangeTable start"<<std::endl;
    int nvs=7;
    int nhs=6;
    int nvt=7;
    int nht=6;
    int nobs=7;
    int naa=13;
    Eigen::VectorXd vs=Eigen::VectorXd::LinSpaced(nvs,0.0,600.0);
    Eigen::VectorXd hs=Eigen::VectorXd::LinSpaced(nhs,0.0,20000.0);
    Eigen::VectorXd vt=Eigen::VectorXd::LinSpaced(nvt,0.0,600.0);
    Eigen::VectorXd ht=Eigen::VectorXd::LinSpaced(nht,0.0,20000.0);
    Eigen::VectorXd obs=Eigen::VectorXd::LinSpaced(nobs,0.0,M_PI);
    Eigen::VectorXd aa=Eigen::VectorXd::LinSpaced(naa,-M_PI,M_PI);
    int numDataPoints=nvs*nhs*nht*nvt*nobs*naa;
    Eigen::MatrixXd indices=Eigen::MatrixXd::Zero(6,numDataPoints);//ColMajor
    Eigen::MatrixXd args=Eigen::MatrixXd::Zero(6,numDataPoints);//ColMajor
    std::cout<<"create indices..."<<std::endl;
    int idx=0;
    for(int aa_i=0;aa_i<naa;aa_i++){
        for(int obs_i=0;obs_i<nobs;obs_i++){
            for(int ht_i=0;ht_i<nht;ht_i++){
                for(int vt_i=0;vt_i<nvt;vt_i++){
                    for(int hs_i=0;hs_i<nhs;hs_i++){
                        for(int vs_i=0;vs_i<nvs;++vs_i){
                            args.block(0,idx,6,1)<<vs(vs_i),hs(hs_i),vt(vt_i),ht(ht_i),obs(obs_i),aa(aa_i);
                            idx++;
                        }
                    }
                }
            }
        }
    }
    std::cout<<"create indices done."<<std::endl;
    int numProcess=std::thread::hardware_concurrency();
    std::vector<std::thread> th;
    std::vector<std::future<Eigen::VectorXd>> f;
    std::vector<int> begins;
    int numPerProc=numDataPoints/numProcess;
    std::cout<<"run in "<<numProcess<<" processes. total="<<numDataPoints<<", perProc="<<numPerProc<<std::endl;
    std::mutex mtx;
    for(int i=0;i<numProcess;++i){
        std::promise<Eigen::VectorXd> p;
        f.emplace_back(p.get_future());
        Eigen::MatrixXd subargs=args.block(0,numPerProc*i,6,(i!=numProcess-1 ? numPerProc : numDataPoints-numPerProc*i));
        th.emplace_back(makeRangeTableSub,std::ref(mtx),std::move(p),modelConfig,instanceConfig,subargs);
    }
    std::cout<<"waiting calculation"<<std::endl;
    std::vector<Eigen::VectorXd> rangeTablePoints_={vs,hs,vt,ht,obs,aa};
    Eigen::VectorXd returned(numDataPoints);
    for(int i=0;i<numProcess;++i){
        try{
            returned.block(numPerProc*i,0,(i!=numProcess-1 ? numPerProc : numDataPoints-numPerProc*i),1)=f[i].get();
        }catch(std::exception& ex){
            {
                std::lock_guard<std::mutex> lock(mtx);
                std::cout<<"exception in proc("<<i<<"): "<<ex.what()<<std::endl;
            }
        }
    }
    Eigen::Tensor<double,6> rangeTable_=Eigen::TensorMap<Eigen::Tensor<double,6>>(returned.data(),nvs,nhs,nvt,nht,nobs,naa);
    for(int i=0;i<numProcess;++i){
        th[i].join();
    }
    std::cout<<"exporting to npz file"<<std::endl;
    auto np=py::module_::import("numpy");
    np.attr("savez")(dstPath,py::arg("vs")=vs,py::arg("hs")=hs,py::arg("vt")=vt,py::arg("ht")=ht,py::arg("obs")=obs,py::arg("aa")=aa,py::arg("ranges")=rangeTable_);
    std::cout<<"makeRangeTable done."<<std::endl;
}
void makeRangeTableSub(std::mutex& m,std::promise<Eigen::VectorXd> p,const nl::json& modelConfig_,const nl::json& instanceConfig_,const Eigen::MatrixXd& args){
    try{
        {
            std::lock_guard<std::mutex> lock(m);
            std::cout<<"subProc started. num="<<args.cols()<<std::endl;
        }
        auto msl=Entity::create<Missile>(modelConfig_,instanceConfig_);
        auto mAcc=msl->manager->copy();
        auto dep=DependencyChecker::create(msl->dependencyChecker);
        nl::json sub={
            {"seed",msl->randomGen()},//Entity
            {"fullName",msl->getFullName()+":Navigator"},//Controller
            {"manager",mAcc},
            {"dependencyChecker",dep},
            {"parent",msl->weak_from_this()}
        };
        auto propNav=msl->manager->generateUnmanagedChild<Controller>("Controller",msl->modelConfig.at("Navigator"),sub);
        msl->controllers["Navigator"]=propNav;
        Eigen::VectorXd ret(args.cols());
        for(int i=0;i<args.cols();++i){
            ret(i)=msl->calcRange(args(0,i),args(1,i),args(2,i),args(3,i),args(4,i),args(5,i));
            {
                std::lock_guard<std::mutex> lock(m);
                std::cout<<i<<"/"<<(int)(args.cols())<<": "<<args(0,i)<<","<<args(1,i)<<","<<args(2,i)<<","<<args(3,i)<<","<<args(4,i)<<","<<args(5,i)<<" range="<<ret(i)<<std::endl;
            }
        }
        p.set_value(ret);
    }catch(...){
        p.set_exception(std::current_exception());
    }
}

void exportMissile(py::module& m)
{
    using namespace pybind11::literals;
    auto cls=EXPOSE_CLASS(Missile);
    cls
    DEF_FUNC(Missile,makeChildren)
    DEF_FUNC(Missile,validate)
    DEF_FUNC(Missile,readConfig)
    DEF_FUNC(Missile,setDependency)
    DEF_FUNC(Missile,perceive)
    DEF_FUNC(Missile,control)
    DEF_FUNC(Missile,behave)
    DEF_FUNC(Missile,kill)
    DEF_FUNC(Missile,calcMotion)
    DEF_FUNC(Missile,calcQ)
    DEF_FUNC(Missile,hitCheck)
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&>(&Missile::getRmax))
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>(&Missile::getRmax))
    DEF_FUNC(Missile,calcRange)
    DEF_FUNC(Missile,calcRangeSub)
    DEF_FUNC(Missile,makeRangeTable)
    DEF_READWRITE(Missile,tMax)
    DEF_READWRITE(Missile,tBurn)
    DEF_READWRITE(Missile,hitD)
    DEF_READWRITE(Missile,minV)
    DEF_READWRITE(Missile,m)
    DEF_READWRITE(Missile,thrust)
    DEF_READWRITE(Missile,maxLoadG)
    DEF_READWRITE(Missile,Sref)
    DEF_READWRITE(Missile,maxA)
    DEF_READWRITE(Missile,l)
    DEF_READWRITE(Missile,d)
    DEF_READWRITE(Missile,ln)
    DEF_READWRITE(Missile,lcg)
    DEF_READWRITE(Missile,lw)
    DEF_READWRITE(Missile,bw)
    DEF_READWRITE(Missile,bt)
    DEF_READWRITE(Missile,thicknessRatio)
    DEF_READWRITE(Missile,Sw)
    DEF_READWRITE(Missile,St)
    DEF_READWRITE(Missile,Isp)
    DEF_READWRITE(Missile,boostMaxG)
    DEF_READWRITE(Missile,boostMaxM)
    DEF_READWRITE(Missile,boostAlt)
    DEF_READWRITE(Missile,accel)
    DEF_READWRITE(Missile,accelScalar)
    DEF_READWRITE(Missile,target)
    DEF_READWRITE(Missile,hasLaunched)
    DEF_READWRITE(Missile,mode)
    DEF_READWRITE(Missile,launchedT)
    DEF_READWRITE(Missile,estTPos)
    DEF_READWRITE(Missile,estTVel)
    DEF_READWRITE(Missile,sensor)
    ;
    py::enum_<Missile::Mode>(cls,"Mode")
    .value("GUIDED",Missile::Mode::GUIDED)
    .value("SELF",Missile::Mode::SELF)
    .value("MEMORY",Missile::Mode::MEMORY)
    ;
}

/*
Based on [Ekker 1994] Appendix. B, but modified a bit.
All values should be provided in MKS units. (angles should be in rad.)
(1) CLaW for subsonic is also calculated by elliptic E, not by DATCOM approximation implemented as CLAWSUB in [Ekker 1994]
(2) Transonic region (M=0.95 to 1.2) is also calculated by same method as either sub/supersonic region to simplify, although it is not so accuarate.
*/
double areapogv(double d,double ln){
    double R=d/4+ln*ln/d;
    double a=R-d/2;
    return ln*sqrt(R*R-ln*ln)+R*R*asin(ln/R)-2*a*ln;
}
double bdycla(double d,double l,double ln,double Mach,double alpha){
	double Sref=M_PI*d*d/4;
	double cdc=sscfdccc(alpha,Mach);
	double Ap=d*(l-ln)+areapogv(d,ln);
    double n;
	if(Mach<=1){
		n=dragfctr(l/d);
    }else{
		n=1.0;
    }
	return k2mk1(l/d)*2+n*Ap*cdc*alpha/Sref;
}
double bdycma_nlpart(double d,double l,double ln,double lcg,double Mach,double alpha){
	double Sref=M_PI*d*d/4;
	double cdc=sscfdccc(alpha,Mach);
	double Ap=d*(l-ln)+areapogv(d,ln);
    double n,cpn,xn;
	if(Mach<1){
		n=dragfctr(l/d);
		cpn=cpogvsb(ln,d);
    }else{
		n=1.0;
		cpn=cpogvemp(d,ln,Mach);
    }
	xn=lcg-cpn;
    return n*Ap*cdc*alpha/Sref/d;
}
double bigkbwa(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	if(b*A<=0){
		return bigkbwsb(r,s);
	}else{
		if(b*A/4<1){
			return bkbwsba(M,A,r,s);
		}else{
			return bkbwspa(M,A,r,s);
        }
    }
}
double bigkbwna(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	if(b*A<=0){
		return bigkbwsb(r,s);
	}else{
		if(b*A/4<1){
			return bkbwsbna(M,A,r,s);
		}else{
			return bkbwspna(M,A,r,s);
        }
    }
}
double bigkbwsb(double r,double s){
	double t=r/s;
    double Kbw;
	if(t>=1){
		Kbw=2.;
	}else{
		double t1=1/t;
		Kbw=(1+pow(t,4))*(0.5*atan(0.5*(t1-t))+M_PI/4)-t*t*((t1-t)+2*atan(t));
		Kbw=(pow(1-t*t,2)-2*Kbw/M_PI)/pow(1-t,2);
    }
	return Kbw;
}
double bigkwb(double r,double s){
	double t=r/s;
    double Kbw;
	if(t>1){
		Kbw=2.;
    }else{
		double t1=1/t;
		Kbw=(1+pow(t,4))*(0.5*atan(0.5*(t1-t))+M_PI/4)-t*t*((t1-t)+2*atan(t));
		Kbw=2*Kbw/M_PI/pow(1-t,2);
    }
	return Kbw;
}
double bkbwsba(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	double t=r/s;
	double z=b*A/4;
	double m=sqrt(1-z*z);
	double E=boost::math::ellint_2(m);
	double t1=2*(1+z)*t/(1-t)+1;
	double t2=pow(z/(z+1),2)/2;
	double Kbw=2*pow(z*t/(1-t),2)*atanh(sqrt(1/t1))+pow(b*A/(b*A+4),2);
	Kbw=t2*sqrt(t1)*(t1+1)-Kbw;
	Kbw=8*E*Kbw/pow(z*M_PI,2);
	return Kbw;
}
double bkbwsbna(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	double t=r/s;
	double z=b*A/4;
	double m=sqrt(1-z*z);
	double E=boost::math::ellint_2(m);
	double t2=z+1;
	double F=2*(1-t)/t/A;
    double Kbw;
	if(b<F){
		double t3=(t+1)/2/t;
		double t4=pow((1-t)/t,2);
		double t1=2*(1-t)/b/A/t;
		Kbw=atanh(sqrt(z*(t1-1)/t3))*t2/sqrt(z)+t1*t1*z*sqrt(z);
		Kbw=t4*t2*(atan(sqrt(1/z))-atan(sqrt((t1-1)/t3)))/b/A-Kbw;
		Kbw=Kbw+t3*sqrt((t1-1)*t3);
		Kbw=16*sqrt(z)*E*Kbw/pow(M_PI,2)/t2/t4;
    }else{
		double z1=sqrt(z);
		Kbw=(8*E/pow(M_PI,2)/(1/t-1))*(z1*atan(1/z1)-z/t2);
    }
	return Kbw;
}
double bkbwspa(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	double t=r/s;
	double z=b*A/4;
	double t1=2*(1+z)*t/(1-t)+1;
	double t2=sqrt(z*z-1);
	double t3=1+z;
	double t4=b*A*t3*t/2/(1-t);
	double Kbw=t2*b*A*pow(t/(1-t),2)*acosh(1+2*(1-t)/b/A/t)+t2/t3+z*acos(1/z)/t3;
	Kbw=t2*sqrt(1+b*A*t/(1-t))/t3-Kbw;
	Kbw=(b*A/(b*A+4))*t1*t1*acos((1+t4)/(z+t4))+Kbw;
	Kbw=Kbw/M_PI/t2;
	return Kbw;
}
double bkbwspna(double M,double A,double r,double s){
	double b=sqrt(abs(M*M-1));
	double t=r/s;
	double z=b*A/4;
	double t1=(1-t)/t;
	double t2=sqrt(z*z-1);
	double F=2*t1/A;
    double Kbw;
	if(b<F){
		Kbw=t2*acosh(2*t1/b/A);
		Kbw=Kbw+t1*t1*acos(1/z)/4;
		Kbw=t1*t1*t2*asin(b*A/t1/2)/b/A-Kbw;
		Kbw=pow((t+1)/2/t,2)*acos((t*A*b/2+4*(1-t)/A/b)/(t+1))+Kbw;
		Kbw=b*A*Kbw/M_PI/t2/pow(t1,2);
    }else{
		Kbw=(2*z/M_PI/(1/t-1))*(M_PI/2-z*acos(1/z)/t2);
    }
	return Kbw;
}
double bsdrgcf(double Mach){
	Eigen::VectorXd p=(Eigen::Matrix<double,9,1>()<<
    	-3.330968357195938e4,
		2.103304015137906e5,
		-5.784664805761514e5,
		9.050874924571224e5,
		-8.811731632140930e5,
		5.466276717656434e5,
		-2.109994072780218e5,
		4.633522170518622e4,
		-4.43198251061754e3).finished();
	return polyval(p,Mach);
}
double bsdrgsp(double Mach){
	static const Eigen::VectorXd p=(Eigen::Matrix<double,9,1>()<<
    	-2.903303569625142e-5,
		8.917615745536690e-4,
		-1.171998069776006e-2,
		8.580630708865064e-2,
		-3.811230276820538e-1,
		1.044387398043697e0,
		-1.697460661376502e0,
		1.405711843047642e0,
		-2.362628974278631e-1).finished();
	return polyval(p,Mach);
}
double cdbbody(double d,double l,double ln,double Mach,double atr){
	double Sref=M_PI*d*d/4;
	double cdc=sscfdccc(atr,Mach);
	double Ap=d*(l-ln)+areapogv(d,ln);
    double n;
	if(Mach<=1){
		n=dragfctr(l/d);
    }else{
		n=1.0;
    }
	return atr*atr*(2*k2mk1(l/d)+n*cdc*Ap*atr/Sref);
}
double cdlcomp(double bw,double Sw,double Mach){
	double beta=sqrt(Mach*Mach-1);
	double Aw=bw*bw/Sw;
	static const Eigen::VectorXd p=(Eigen::Matrix<double,8,1>()<<
		-1.131676150597421e0,
		7.455524067331654e0,
		-1.959790137335054e1,
		2.617725155474472e1,
		-1.909733706084593e1,
		8.093378527305321e0,
		-1.217830853307086e0,
		5.365069933169143e-1).finished();
	return polyval(p,beta*Aw/4);
}
double cdlwing(double bw,double Sw,double Mach_,double atr){
	double Aw=bw*bw/Sw;
    double Mach=Mach_>0?Mach_:0.0001;
	if(atr==0){
		return 0.0;
    }
	if(Mach<=1){
        double kappaw=0.85;
        double lc2w=atan(2/Aw);
		double CLaW=clawsub(Mach,Aw,lc2w,kappaw);
		return CLaW*atr*atr/1.1;
	}else{
		double betaw=sqrt(Mach*Mach-1);
		if(betaw*Aw/4>=1){
			return 4*atr*atr/betaw;
        }else{
			double corr=cdlcomp(bw,Sw,Mach);
			double CLaW=clawsup(Mach,Aw);
			return 3*pow(CLaW*atr,2)*corr/M_PI/Aw;
        }
    }
}
double cdobody(double d,double l,double ln,double Mach,double alt,double nf,bool pwroff){
	double Sref=M_PI*d*d/4.;
	double Swet=surfogv1(d,l)+(l-ln)*M_PI*d;
    double cdo,CDF;
	if(Mach<=0){
		cdo=0.0;
    }else if(Mach<=1){
		double Mach1=std::min(0.6,Mach);
		double cf=nf*cfturbfp(l,Mach1,alt);
		CDF=cf*Swet/Sref;
		double CDP=cf*(60/pow(l/d,3)+0.0025*l/d)*Swet/Sref;
		cdo=CDF+CDP;
    }else{
		double cf=nf*cfturbfp(l,Mach,alt);
		double CDW=wvdrgogv(d,ln,Mach);
		double CDF=cf*Swet/Sref;
        double CDP;
		if(Mach<1.2){
			double cf=nf*cfturbfp(l,0.6,alt);
			double CDPp6=cf*(60/pow(l/d,3)+0.0025*l/d)*Swet/Sref;
			CDP=CDPp6*(1.2-Mach)/0.2;
        }else{
			CDP=0;
        }
		cdo=CDF+CDW+CDP;
    }
	if(pwroff){
        double cdb;
		if(Mach<=0){
			cdb=0;
		}else if(Mach<=1){
			cdb=0.029/sqrt(CDF);
			if(Mach>0.6){
				cdb=cdb+bsdrgcf(Mach);
            }
        }else{
			cdb=-bsdrgsp(Mach);
        }
		cdo=cdo+cdb;
    }
	return cdo;
}
double cdowbt(double d,double l,double ln,double bw,double Sw,double thickw,double bt,double St,double thickt,double Mach,double alt,double nf,bool pwroff){
	double Sref=M_PI*d*d/4.;
	if(Mach<=0.95 || Mach>=1.1){
		double cdob=cdobody(d,l,ln,Mach,alt,nf,pwroff);
		double cdow=cdowing(bw,Sw,thickw,Mach,alt,nf);
		double cdot=cdowing(bt,St,thickt,Mach,alt,nf);
		return cdob+2*cdow*Sw/Sref+2*cdot*St/Sref;
    }else{
		double cdop95=cdowbt(d,l,ln,bw,Sw,thickw,bt,St,thickt,0.95,alt,nf,pwroff);
		double cdo1p1=cdowbt(d,l,ln,bw,Sw,thickw,bt,St,thickt,1.1,alt,nf,pwroff);
		double dCdM=(cdo1p1-cdop95)/0.15;
		return std::min(cdo1p1,dCdM*(Mach-0.95)+cdop95);
    }
}
double cdowing(double bw,double Sw,double thick,double Mach_,double alt,double nf){
    double Mach=Mach_==1.0?0.999999:Mach_;
	double MAC=(2.0/3.0)*2*Sw/bw;
	if(Mach==0){
		return 0.0;
    }else if(Mach<=1){
		double cf=nf*cfturbfp(MAC,Mach,alt);
		return 2*cf*(1+1.2*thick+60*pow(thick,4));
    }else{
		double cf=nf*cfturbfp(MAC,Mach,alt);
		double beta=sqrt(Mach*Mach-1.);
		double B=4.;
		double m=bw*bw/Sw/4;
        double cdw;
		if(m*beta>=1){
			cdw=B*thick*thick/beta;
        }else{
			cdw=B*m*thick*thick;
        }
		return 2*cf+cdw;
    }
}
double cdtrim(double d,double l,double ln,double bw,double Sw,double thickw,double bt,double St,double thickt,double Mach,double alt,double atr,double dtr,double nf,bool pwroff){
	double Sref=M_PI*d*d/4.;
	double CDoB=cdobody(d,l,ln,Mach,alt,nf,pwroff);
	double CDoW=cdowing(bw,Sw,thickw,Mach,alt,nf);
	double CDoT=cdowing(bt,St,thickt,Mach,alt,nf);
	double CDoWB=CDoB+2*CDoW*Sw/Sref+CDoT*St/Sref;
	double CDLW=cdlwing(bw,Sw,Mach,atr);
	double CDBa=cdbbody(d,l,ln,Mach,atr);
	double CDiWB=CDLW*Sw/Sref+CDBa;
	double Mt=0.95*Mach;
	double CDLT=cdlwing(bt,St,Mt,dtr);
	double CDT=CDoT+CDLT;
	double CLd=cldwbt(d,bt,St,Mt)*Sref/St;
	double CLT=CLd*dtr;
	double nt=pow(Mt/Mach,2);
	double CDTR=(CDT*cos(dtr)+CLT*sin(dtr))*St*nt/Sref;
	return CDoWB+CDiWB+CDTR;
}
double cfturbfp(double chardim,double Mach,double alt){
    std::vector<double> atm=atmosphere(alt);
    double Vs=atm[1];
    double nu=atm[5];
	double Rn=chardim*Mach*Vs/nu;
	double cfi=0.455/pow(log10(Rn),2.58);
    double cf;
	if(Mach<=1){
		cf=cfi/(1+0.08*Mach*Mach);
    }else{
		cf=cfi/pow(1+0.144*Mach*Mach,0.65);
    }
	return cf;
}
std::pair<double,double> clacma(double d,double l,double ln,double lcg,double bw,double Sw,double lw,double bt,double St,double Mach,double alpha){
	//combine CLAMSLSB, CLAMSLSP and CLAWBT in a single function
	double Sref=M_PI*d*d/4.;
	//Body
	double CLaN=bdycla(d,l,ln,Mach,alpha);
    double cpn,xn;
	if(Mach<1){
		cpn=cpogvsb(ln,d);
    }else{
		cpn=cpogvemp(d,ln,Mach);
    }
	xn=lcg-cpn;
	double CMaNd=xn*CLaN;
	//Wing
	double Aw=bw*bw/Sw;
	double sw=0.5*(d+bw);
	double crw=2*Sw/bw;
    double CLaW,CLaW0,Kbw,Kwb,xbw,xwb;
	if(Mach<1){
        double lc2w=atan(2/Aw);
        double kappaw=0.85;
        CLaW=clawsub(Mach,Aw,lc2w,kappaw);
		CLaW0=clawsub(0,Aw,lc2w,kappaw);
		Kbw=bigkbwsb(d/2,sw);
		Kwb=bigkwb(d/2,sw);
		xbw=lw+xcrbwsub(Mach,Aw,d/2,sw)*crw-lcg;
		xwb=lw+xcrwbsub(Mach,Aw)*crw-lcg;
    }else{
        CLaW=clawsup(Mach,Aw);
		Kbw=bigkbwa(Mach,Aw,d/2,sw);
		Kwb=bigkwb(d/2,sw);
		double betaw=sqrt(Mach*Mach-1);
		double xcrwb=xcrwba(d/2,sw);
        double xcrbw;
		if(betaw*Aw>=0){
			xcrbw=xcrbwabh(Mach,d/2,crw);
        }else{
			xcrbw=xcrbwabl(Mach,Aw,d/2,sw);
        }
		xbw=lw+xcrbw*crw-lcg;
		xwb=lw+xcrwb*crw-lcg;
    }
	//Tail
	double Mt=0.95*Mach;
	double nt=pow(Mt/Mach,2);
	double At=bt*bt/St;
	double st=0.5*(d+bt);
	double crt=2*St/bt;
    double CLaT,Kbt,Ktb,xbt,xtb;
	if(Mt<1){
        double lc2t=atan(2/At);
        double kappat=0.85;
	    CLaT=clawsub(Mt,At,lc2t,kappat);
		Kbt=bigkbwsb(d/2,st);
		Ktb=bigkwb(d/2,st);
    }else{
	    CLaT=clawsup(Mt,At);
		Kbt=bigkbwna(Mt,At,d/2,st);
		Ktb=bigkwb(d/2,st);
    }
	if(Mach<1){
		double lt=l-crt;
		xbt=lt+xcrbwsub(Mt,At,d/2,st)*crt-lcg;
		xtb=lt+xcrwbsub(Mt,At)*crt-lcg;
    }else{
		double lt=l-crt;
		double betat=sqrt(abs(Mt*Mt-1));
		double xcrtba=xcrwba(d/2,st);
        double xcrbta;
		if(betat*At>=0){
			xcrbta=xcrbwnab(Mt,d/2,crt);
        }else{
			xcrbta=xcrbwabl(Mt,At,d/2,st);
        }
		xbt=lt*xcrbta*crt-lcg;
		xtb=lt*xcrtba*crt-lcg;
    }
	//Downwash
    double deda;
	if(Mach<1){
		double lH=l-St/bt-(lw+Sw/bw);
		double lc4w=atan(3/Aw);
		deda=dedasub(Aw,bw,lc4w,0,lH,0)*CLaW/CLaW0;
    }else{
		double SWPwing=atan(2*crw/bw);
		double lt=l-crt;
		double x0=(lt-(lw+crw))/crw;
		deda=Kwb*dedasup(Mach,SWPwing,x0);
    }
	double cla=CLaN+(Kbw+Kwb)*CLaW*Sw/Sref+(Kbt+Ktb)*CLaT*(1-deda)*nt*St/Sref;
	double cmad=CMaNd-(Kbw*xbw+Kwb*xwb)*CLaW*Sw/Sref-(Kbt*xbt+Ktb*xtb)*CLaT*(1-deda)*nt*St/Sref;
	return std::make_pair(cla,cmad/d);
}
double clawsub(double M,double A,double lc2,double kappa){
	double b=sqrt(abs(M*M-1));
    return 2*M_PI*A/(2+sqrt(pow(A*b/kappa,2)*(1+pow(tan(lc2)/b,2))+4));
}
double clawsup(double M,double A){
	double b=sqrt(abs(M*M-1));
	if(b*A/4>=1){
		return 4/b;
    }else{
		double E=boost::math::ellint_2(sqrt(1-pow(b*A/4,2)));
		return M_PI*A/2/E;
    }
}
std::pair<double,double> cldcmd(double d,double l,double ln,double lcg,double bt,double St,double Mach){
	//Combining clawbt and cmdwbt in a single function
	double Mt=0.95*Mach;
	double nt=pow(Mt/Mach,2);
	double Sref=M_PI*d*d/4;
	double At=bt*bt/St;
	double st=0.5*(d+bt);
	double crt=2*St/bt;
	double lt=l-crt;
	double kbt=smlkbw(d/2,st);
	double ktb=smlkwb(d/2,st);
    double CLaT,xcrbt,xcrtb;
	if(Mt<1){
        double lc2t=atan(2/At);
        double kappat=0.85;
        CLaT=clawsub(Mt,At,lc2t,kappat);
		xcrbt=xcrbwsub(Mt,At,d/2,st);
		xcrtb=xcrwbsub(Mt,At);
    }else{
        CLaT=clawsup(Mt,At);
		double betat=sqrt(Mt*Mt-1);
		if(betat*At<0){
			xcrbt=xcrbwabl(Mt,At,d/2,st);
		}else{
			xcrbt=xcrbwnab(Mt,d/2,crt);
        }
		xcrtb=xcrwbd(d/2,st);
    }
	double lbt=lt+xcrbt*crt;
	double ltb=lt+xcrtb*crt;
	double xbt=lbt-lcg;
	double xtb=ltb-lcg;
	double cld=CLaT*nt*(kbt+ktb)*St/Sref;
	double cmd=-CLaT*nt*(kbt*xbt+ktb*xtb)*St/Sref/d;
	return std::make_pair(cld,cmd);
}
double cldwbt(double d,double bt,double St,double Mach){
	double Mt=0.95*Mach;
	double nt=pow(Mt/Mach,2);
	double Sref=M_PI*d*d/4;
	double At=bt*bt/St;
	double st=0.5*(d+bt);
    double CLaT;
    if(Mt<1){
        double lc2t=atan(2/At);
        double kappat=0.85;
	    CLaT=clawsub(Mt,At,lc2t,kappat);
    }else{
	    CLaT=clawsup(Mt,At);
    }
	double kbt=smlkbw(d/2,st);
	double ktb=smlkwb(d/2,st);
	return CLaT*nt*(kbt+ktb)*St/Sref;
}
double cpogvemp(double d,double ln,double M){
	double sigma=2*atan(d/2/ln)*180/M_PI;
	double P=(0.083+0.096/pow(M,2))*pow(sigma/10,1.69);
	return 0.5*(50*(M+18)+7*M*M*P*(5*M-18))*ln/(40*(M+18)+7*M*M*P*(4*M-3));
}
double cpogvsb(double ln,double d){
	double Vn=vologv1(ln,d);
	return ln-4*Vn/M_PI/pow(d,2);
}
double dedasub(double A,double b,double lc4,double lam,double lH,double hH){
	double KA=1./A-1./(1.+pow(A,1.7));
	double Kl=(10.-3.*lam)/7.;
	double KH=(1-abs(hH/b))/pow(2*lH/b,1./3.);
	return 4.44*pow(KA*Kl*KH*sqrt(cos(lc4)),1.19);
}
double dedasup(double M,double lam,double x0){
	double psi=M_PI/2-lam;
	double t0=tan(psi)*sqrt(M*M-1);
    double e2,e4,e6,e8;
	if(x0<1.0){
		double e2=0.81;
    }else if(x0<=1.3){
		static const Eigen::VectorXd p21=(Eigen::Matrix<double,4,1>()<<
			-1.666666666666488e0,
			5.999999999999325e0,
			-6.783333333332485e0,
			3.259999999999646e0).finished();
		e2=polyval(p21,x0);
    }else if(x0<2.3){
		static const Eigen::VectorXd p22=(Eigen::Matrix<double,9,1>()<<
        	-1.279239755663028e2,
			1.843272381677282e3,
			-1.155286798315866e4,
			4.113252460613160e4,
			-9.098009805564612e4,
			1.280057606805492e5,
			-1.118657697776747e5,
			5.551390292092566e4,
			-1.197584408390502e4).finished();
		e2=polyval(p22,x0);
    }else{
		e2=0.97;
    }
	if(x0<1.0){
		e4=0.65;
    }else if(x0<=1.6){
		static const Eigen::VectorXd p41=(Eigen::Matrix<double,5,1>()<<
        	-4.166666666665245e0,
			2.083333333332625e1,
			-3.845833333332031e1,
			3.134166666665624e1,
			-8.899999999996956e0).finished();
		e4=polyval(p41,x0);
    }else if(x0<=3.0){
		static const Eigen::VectorXd p42=(Eigen::Matrix<double,9,1>()<<
        	4.475474425067136e0,
			-8.198016093532951e1,
			6.529374943388477e2,
			-2.952920552556513e3,
			8.292996506025491e3,
			-1.480813974021849e4,
			1.641631929076350e4,
			-1.032958858524409e4,
			2.825103492654550e3).finished();
		e4=polyval(p42,x0);
    }else{
		e4=0.87;
    }
	if(x0<1.0){
		e6=0.54;
    }else if(x0<=1.4){
		static const Eigen::VectorXd p61=(Eigen::Matrix<double,4,1>()<<
        	1.600881353985564e-14,
			-6.387361790702744e-14,
			1.000000000000832e-1,
			4.399999999999644e-1).finished();
		e6=polyval(p61,x0);
    }else if(x0<=1.8){
		static const Eigen::VectorXd p62=(Eigen::Matrix<double,5,1>()<<
        	-3.333333333309070e1,
			2.116666666651215e2,
			-5.026666666629868e2,
			5.294833333294496e2,
			-2.082299999984673e2).finished();
		e6=polyval(p62,x0);
    }else if(x0<3.6){
		static const Eigen::VectorXd p63=(Eigen::Matrix<double,7,1>()<<
        	6.35640889426016e-3,
			-5.240723295023863e-2,
			-2.580914111953288e-2,
			1.556718856871693e0,
			-6.218769329470563e0,
			1.005756492417748e1,
			-5.269710040962651e0).finished();
		e6=polyval(p63,x0);
    }else{
		e6=0.79;
    }
	if(x0<1.0){
		e8=0.44;
    }else if(x0<=1.4){
		static const Eigen::VectorXd p81=(Eigen::Matrix<double,5,1>()<<
        	-1.666666666665405e1,
			7.999999999993737e1,
			-1.433333333332168e2,
			1.136499999999038e2,
			-3.320999999997028e1).finished();
		e8=polyval(p81,x0);
    }else if(x0<=2.2){
        static const Eigen::VectorXd p82=(Eigen::Matrix<double,9,1>()<<
			-8.680554787360018e2,
			1.252976079851500e4,
			-7.886457638672026e4,
			2.827083084908162e5,
			-6.312798925797358e5,
			8.991348588842052e5,
			-7.977027436345048e5,
			4.030393822169418e5,
			-8.878791230326184e4).finished();
		e8=polyval(p82,x0);
    }else if(x0<3.4){
		static const Eigen::VectorXd p83=(Eigen::Matrix<double,9,1>()<<
        	3.866380123250028e0,
			-9.169392757855618e1,
			9.469642944017384e2,
			-5.5620644279886374e3,
			2.032063141263390e4,
			-4.728372988378842e4,
			6.842861819191089e4,
			-5.630916695234881e4,
			2.017213892626900e4).finished();
		e8=polyval(p83,x0);
    }else{
		e8=0.7;
    }
    double dedaw;
	if(t0<=0.2){
		dedaw=(e2-e4)*(0.2-t0)/0.2+e2;
    }else if(t0<=0.4){
		dedaw=(e2-e4)*(0.4-t0)/0.2+e4;
    }else if(t0<=0.6){
		dedaw=(e4-e6)*(0.6-t0)/0.2+e6;
    }else if(t0<=0.8){
		dedaw=(e6-e8)*(0.8-t0)/0.2+e8;
    }else{//if(t0>0.8){
		dedaw=e8-(e6-e8)*(t0-0.8)/0.2;
    }
	return std::max(0.0,dedaw);
}
double dragfctr(double fineness){
	static const Eigen::VectorXd p=(Eigen::Matrix<double,8,1>()<<
    	-3.610649936485454e-12,
		8.309751611041349e-10,
		-7.347789500313429e-8,
		3.037472628776377e-6,
		-5.190776944854139e-5,
		-1.773312264112830e-4,
		2.135202437974324e-2,
		5.187105185342232e-1).finished();
	return polyval(p,fineness);
}
double k2mk1(double fineness){
	static const Eigen::VectorXd pa=(Eigen::Matrix<double,8,1>()<<
    	-6.3424e-7,
		3.1805e-5,
		-6.0351e-4,
		5.0080e-3,
		-9.9444e-3,
		-1.1439e-1,
		7.7981e-1,
		-6.6077e-1).finished();
	static const Eigen::VectorXd pb=(Eigen::Matrix<double,2,1>()<<1.6667e-3,9.4667e-1).finished();
	if(fineness<0){
		fineness=0;
    }
    double amf;
    if(fineness<14){
		amf=polyval(pa,fineness);
    }else{
		amf=polyval(pb,fineness);
    }
	return std::max(0.0,amf);
}
double smlkbw(double r,double s){
	return bigkwb(r,s)-smlkwb(r,s);
}
double smlkwb(double r,double s){
	double t=s/r;
    double kwb;
	if(t<=1){
		kwb=1;
    }else{
		kwb=pow(M_PI*(t+1)/t,2)/4+M_PI*pow((t*t+1)/(t*(t-1)),2)*asin((t*t-1)/(t*t+1));
		kwb=kwb-2*M_PI*(t+1)/t/(t-1)+pow((t*t+1)/(t*(t-1)),2)*pow(asin((t*t-1)/(t*t+1)),2);
		kwb=kwb-4*(t+1)*asin((t*t-1)/(t*t+1))/t/(t-1)+8*log((t*t+1)/2/t)/pow(t-1,2);
		kwb=kwb/pow(M_PI,2);
    }
	return kwb;
}
double sscfdccc(double alpha,double M){
    static const Eigen::VectorXd pa=(Eigen::Matrix<double,5,1>()<<-2.5513,4.9961,-1.5000,0.1394,1.1967).finished();
	static const Eigen::VectorXd pb=(Eigen::Matrix<double,6,1>()<<-2.1943e2,9.0057e2,-1.4619e3,1.1695e3,-4.5889e2,7.1919e1).finished();
	static const Eigen::VectorXd pc=(Eigen::Matrix<double,5,1>()<<154.7931,-553.2488,737.6712,-434.1301,96.7047).finished();
	static const Eigen::VectorXd pd=(Eigen::Matrix<double,6,1>()<<-3.4916,7.9077,-6.1199,2.3609,-7.8519e-2,1.2400).finished();
	double k=sin(alpha)*M;
	if(k>1){
		k=1/k;
		if(k>0.8){
			return polyval(pc,k);
        }else{
			return polyval(pd,k);
        }
    }else{
		if(k>0.6){
			return polyval(pb,k);
        }else{
			return polyval(pa,k);
        }
    }
}
double surfogv1(double diameter,double length){
	double ldr=length/diameter;
	double ldr2=ldr*ldr;
	double surf1=asin(ldr/(ldr2+0.25));
	return M_PI*pow(diameter,2)*(pow(ldr2+0.25,2)*surf1-ldr*(ldr2-0.25));
}
double vologv1(double logv,double dbase){
	double l=logv;
	double d=dbase;
	double R=d/4+l*l/d;
	double vol=(R-d/2)*(l*sqrt(R*R-l*l)+R*R*asin(l/R));
	vol=M_PI*(l*(2*R*R-R*d+d*d/4)-pow(l,3)/3-vol);
	return vol;
}
double wvdrgogv(double diameter,double length,double Mach){
	double sigma=2*(180/M_PI)*atan(diameter/2/length);
	double P=(0.083+0.096/Mach*Mach)*pow(sigma/10,1.69);
	double lod2=pow(length/diameter,2);
	return P*(1-(392*lod2-32)/28/lod2/(Mach+18));
}
double xcrbwabh(double M,double r,double cr){
	//supersonic, with afterbody and low aspect ratio
	double b=sqrt(abs(M*M-1));
	double t=2*b*r/cr;
	static const Eigen::VectorXd p=(Eigen::Matrix<double,4,1>()<<1.7611e-2,-1.0474e-1,5.9054e-1,5.0298e-1).finished();
	return polyval(p,t);
}
double xcrbwabl(double M,double A,double r,double s){
	//supersonic, with afterbody and high aspect ratio
	double b=sqrt(abs(M*M-1));
	static const Eigen::VectorXd p=(Eigen::Matrix<double,4,1>()<<1.7976e0,-8.2857e-1,4.3667e-1,-5.7381e-17).finished();
	double t1=b*A;
	double t2=r/s;
	double slope=polyval(p,t2);
	return slope*t1+0.5;
}
double xcrbwnab(double M,double r,double cr){
	//supersonic and without afterbody
	double b=sqrt(abs(M*M-1));
	double t=2*b*r/cr;
	static const Eigen::VectorXd p=(Eigen::Matrix<double,8,1>()<<8.3600e0,-3.0112e1,4.3283e1,-3.1734e1,1.2746e1,-3.0908e0,7.1424e-1,5.0019e-1).finished();
	if(t>0.9){
		return 2./3.;
    }else{
		return polyval(p,t);
    }
}
double xcrbwsub(double M,double A,double r,double s){
	//subsonic
	double b=sqrt(abs(1-M*M));
	double t=r/s;
    double rs6,rs4,rs2,rs0;
	if(b*A<4){
		static const Eigen::VectorXd p6=(Eigen::Matrix<double,5,1>()<<
        	-1.503003360588808e-4,
			1.500533636756856e-3,
			-5.843498893035511e-3,
			1.266301279846288e-2,
			4.997093223254211e-1).finished();
		rs6=polyval(p6,b*A);
        static const Eigen::VectorXd p4=(Eigen::Matrix<double,10,1>()<<
			1.439562405737517e-4,
			-2.577574370693720e-3,
			1.915260292425828e-2,
			-7.636557567970656e-2,
			1.764081283046741e-1,
			-2.379409374985050e-1,
			1.805435508411277e-1,
			-7.176958947911805e-2,
			5.130816740444732e-3,
			4.999682326019724e-1).finished();
		rs4=polyval(p4,b*A);
        static const Eigen::VectorXd p2=(Eigen::Matrix<double,9,1>()<<
			1.870466827291419e-4,
			-3.055345401484121e-3,
			2.012956338935916e-2,
			-6.808482446989989e-2,
			1.241295486321807e-1,
			-1.160715264335058e-1,
			4.940441636896099e-2,
			-4.087742141651518e-2,
			5.000384633792437e-1).finished();
		rs2=polyval(p2,b*A);
    }else{
		rs6=0.5+2.6*0.05/9;
		rs4=0.45+4*0.05/9;
		rs2=0.4+1.5*0.05/9;
    }
	if(b*A>=7){
		rs0=0.25;
    }else{
        static const Eigen::VectorXd p0=(Eigen::Matrix<double,7,1>()<<
        	1.368278964039268e-5,
			-2.870181449710582e-4,
			2.101837326902790e-3,
			-6.106506978595964e-3,
			1.093840736149155e-2,
			-7.485137721564938e-2,
			5.003830336176011e-1).finished();
		rs0=polyval(p0,b*A);
    }
	if(t>0.6){
		return rs6+(rs6-rs4)*(t-0.6)/0.2;
    }else if(t>0.4){
		return rs4+(rs6-rs4)*(t-0.4)/0.2;
    }else if(t>0.2){
		return rs2+(rs4-rs2)*(t-0.2)/0.2;
    }else{
		return rs0+(rs2-rs0)*t/0.2;
    }
}
double xcrw(double M,double A){
	double b=sqrt(abs(M*M-1));
	double mb=b*A;
	return 2./3.;
}
double xcrwba(double r,double s){
	double t=r/s;
	static const Eigen::VectorXd p=(Eigen::Matrix<double,4,1>()<<-0.0822,0.2009,-0.1190,0.6667).finished();
	return polyval(p,t);
}
double xcrwbd(double r,double s){
	double t=r/s;
	static const Eigen::VectorXd p=(Eigen::Matrix<double,8,1>()<<0.8716,-3.0894,4.3106,-3.1344,1.3829,-0.3885,0.0473,0.6669).finished();
	return polyval(p,t);
}
double xcrwbsub(double M,double A){
	double b=sqrt(abs(1-M*M));
	if(b*A<2){
        static const Eigen::VectorXd p=(Eigen::Matrix<double,6,1>()<<
			-1.130484330484368e-2,
			5.420357420357615e-2,
			-1.073193473193506e-1,
			1.446257446257462e-1,
			-1.627277907277905e-1,
			6.555089355089354e-1).finished();
		return polyval(p,b*A);
    }else if(b*A>=5){
		return 0.5+3.5/90;
    }else{
        static const Eigen::VectorXd p=(Eigen::Matrix<double,3,1>()<<
			-1.785830762908004e-17,-5.555555555555476e-3,5.666666666666665e-1).finished();
		return polyval(p,b*A);
    }
}