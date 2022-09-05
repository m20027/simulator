#include "R3AgentSample01.h"
#include <algorithm>
#include <iomanip>
#include <pybind11/numpy.h>
#include <pybind11/eigen.h>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/MathUtility.h>
#include <ASRCAISim1/Units.h>
#include <ASRCAISim1/MassPointFighter.h>
#include <ASRCAISim1/CoordinatedFighter.h>
#include <ASRCAISim1/Missile.h>
#include <ASRCAISim1/Track.h>
#include <ASRCAISim1/SimulationManager.h>
#include <ASRCAISim1/Ruler.h>
using namespace util;
R3AgentSample01::R3AgentSample01(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:SingleAssetAgent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    turnScale=deg2rad(getValueFromJsonKRD(modelConfig,"turnScale",randomGen,90));
    turnDim=getValueFromJsonKRD(modelConfig,"turnDim",randomGen,7);
    pitchScale=deg2rad(getValueFromJsonKRD(modelConfig,"pitchScale",randomGen,45));
    pitchDim=getValueFromJsonKRD(modelConfig,"pitchDim",randomGen,7);
    accelScale=getValueFromJsonKRD(modelConfig,"accelScale",randomGen,30);
    accelDim=getValueFromJsonKRD(modelConfig,"accelDim",randomGen,7);
    hMin=getValueFromJsonKRD(modelConfig,"hMin",randomGen,1000);
    hMax=getValueFromJsonKRD(modelConfig,"hMax",randomGen,15000);
    dOutLimitRatio=getValueFromJsonKRD(modelConfig,"dOutLimitRatio",randomGen,0.9);
    rangeNormalizer=getValueFromJsonKRD(modelConfig,"rangeNormalizer",randomGen,100000.0);
    fgtrVelNormalizer=getValueFromJsonKRD(modelConfig,"fgtrVelNormalizer",randomGen,300);
    mslVelNormalizer=getValueFromJsonKRD(modelConfig,"mslVelNormalizer",randomGen,2000);
    maxTrackNum=getValueFromJsonKRD<std::map<std::string,int>>(modelConfig,"maxTrackNum",randomGen,{{"Friend",0},{"Enemy",2}});
    maxMissileNum=getValueFromJsonKRD<std::map<std::string,int>>(modelConfig,"maxMissileNum",randomGen,{{"Friend",0},{"Enemy",1}});
    pastPoints=getValueFromJsonKRD<std::vector<int>>(modelConfig,"pastPoints",randomGen,std::vector<int>());
    minimumV=getValueFromJsonKRD(modelConfig,"minimumV",randomGen,150.0);
    minimumRecoveryV=getValueFromJsonKRD(modelConfig,"minimumRecoveryV",randomGen,180.0);
    minimumRecoveryDstV=getValueFromJsonKRD(modelConfig,"minimumRecoveryDstV",randomGen,200.0);
    maxSimulShot=getValueFromJsonKRD(modelConfig,"maxSimulShot",randomGen,2);
    pastData.clear();
    singleDim=8+9*maxTrackNum["Friend"]+8*maxTrackNum["Enemy"]+11*maxMissileNum["Friend"]+3*maxMissileNum["Enemy"];
    if(pastPoints.size()>0){
        int maxT=*std::max_element(pastPoints.begin(),pastPoints.end());
        for(int i=0;i<maxT;++i){
            pastData.push_back(Eigen::VectorXf::Zero(singleDim));
        }
    }
    lastTrackInfo.clear();
    launchFlag=false;
    velRecovery=false;
    target=Track3D();
}
R3AgentSample01::~R3AgentSample01(){}

void R3AgentSample01::validate(){
    auto rulerObs=manager->getRuler()->observables;
    dOut=rulerObs.at("dOut");
    dLine=rulerObs.at("dLine");
    hLim=rulerObs.at("hLim");
    std::string eastSider=rulerObs.at("eastSider");
    if(getTeam()==eastSider){
        xyInv<<1,1,1;
    }else{
        xyInv<<-1,-1,1;
    }
    if(parent->isinstance<CoordinatedFighter>()){
        std::dynamic_pointer_cast<FighterAccessor>(parent)->setFlightControllerMode("fromDirAndVel");
    }else if(parent->isinstance<MassPointFighter>()){
        const nl::json& fSpec=parent->observables.at("/spec/dynamics"_json_pointer);
        omegaScale<<1./fSpec.at("rollMax").get<double>(),1./fSpec.at("pitchMax").get<double>(),1./fSpec.at("yawMax").get<double>();
    }else{
        throw std::runtime_error("R3AgentSample01 accepts only MassPointFighrer or CoordinatedFighter.");
    }
    MotionState myMotion(parent->observables.at("motion"));
    baseV=dstV=myMotion.vel.norm();
    dstDir<<0,-xyInv(1),0;
    lastAction=VecX<long>::Zero(4);
    lastAction<<turnDim/2,pitchDim/2,accelDim/2,0;
}
py::object R3AgentSample01::observation_space(){
    float floatLow=std::numeric_limits<float>::lowest();
    float floatHigh=std::numeric_limits<float>::max();
    Eigen::VectorXf self_low(8);self_low<<floatLow,floatLow,floatLow,floatLow,-1,-1,-1,0;
    Eigen::VectorXf self_high(8);self_high<<floatHigh,floatHigh,floatHigh,floatHigh,1,1,1,floatHigh;
    Eigen::VectorXf friend_low(9);friend_low<<floatLow,-1,-1,-1,floatLow,-1,-1,-1,0;
	Eigen::VectorXf friend_high(9);friend_high<<floatHigh,1,1,1,floatHigh,1,1,1,floatHigh;
	Eigen::VectorXf enemy_low(8);enemy_low<<floatLow,-1,-1,-1,floatLow,-1,-1,-1;
	Eigen::VectorXf enemy_high(8);enemy_high<<floatHigh,1,1,1,floatHigh,1,1,1;
	Eigen::VectorXf msl_friend_low(11);msl_friend_low<<floatLow,-1,-1,-1,floatLow,-1,-1,-1,0,0,0;
	Eigen::VectorXf msl_friend_high(11);msl_friend_high<<floatHigh,1,1,1,floatHigh,1,1,1,1,1,1;
	Eigen::VectorXf msl_enemy_low(3);msl_enemy_low<<-1,-1,-1;
	Eigen::VectorXf msl_enemy_high(3);msl_enemy_high<<1,1,1;
    Eigen::VectorXf singleLows=Eigen::VectorXf::Zero(singleDim);
    Eigen::VectorXf singleHighs=Eigen::VectorXf::Zero(singleDim);
    int ofs=0;
    singleLows.block(ofs+0,0,self_low.size(),1)=self_low;
    singleHighs.block(ofs+0,0,self_high.size(),1)=self_high;
    ofs+=self_low.size();
    for(int i=0;i<maxTrackNum["Friend"];++i){
        singleLows.block(ofs+0,0,friend_low.size(),1)=friend_low;
        singleHighs.block(ofs+0,0,friend_high.size(),1)=friend_high;
        ofs+=friend_low.size();
    }
    for(int i=0;i<maxTrackNum["Enemy"];++i){
        singleLows.block(ofs+0,0,enemy_low.size(),1)=enemy_low;
        singleHighs.block(ofs+0,0,enemy_high.size(),1)=enemy_high;
        ofs+=enemy_low.size();
    }
    for(int i=0;i<maxMissileNum["Friend"];++i){
        singleLows.block(ofs+0,0,msl_friend_low.size(),1)=msl_friend_low;
        singleHighs.block(ofs+0,0,msl_friend_high.size(),1)=msl_friend_high;
        ofs+=msl_friend_low.size();
    }
    for(int i=0;i<maxMissileNum["Enemy"];++i){
        singleLows.block(ofs+0,0,msl_enemy_low.size(),1)=msl_enemy_low;
        singleHighs.block(ofs+0,0,msl_enemy_high.size(),1)=msl_enemy_high;
        ofs+=msl_enemy_low.size();
    }
    Eigen::VectorXf lows=Eigen::VectorXf::Zero(singleDim*(1+pastPoints.size()));
    Eigen::VectorXf highs=Eigen::VectorXf::Zero(singleDim*(1+pastPoints.size()));
    ofs=0;
    for(int i=0;i<1+pastPoints.size();++i){
        lows.block(ofs+0,0,singleDim,1)=singleLows;
        highs.block(ofs+0,0,singleDim,1)=singleHighs;
        ofs+=singleDim;
    }
    py::module_ spaces=py::module_::import("gym.spaces");
    py::dict kwargs;
    kwargs["low"]=lows;
    kwargs["high"]=highs;
    kwargs["dtype"]=py::dtype::of<float>();
    return spaces.attr("Box")(*py::tuple(),**kwargs);
}
py::object R3AgentSample01::makeObs(){
    int count=round(manager->getTickCount()/manager->getAgentInterval());
    Eigen::VectorXf current=makeSingleObs();
    int idx=0;
    if(pastPoints.size()==0){
        return py::cast(current);
    }
    if(count==0){
        //初回は過去の仮想データを生成(誘導弾なし、敵側の情報なし)
        std::vector<Eigen::Vector3d> friendPos0,friendVel;
        Eigen::Vector3d myPos0=parent->observables.at("/motion/pos"_json_pointer);
        Eigen::Vector3d myVel=parent->observables.at("/motion/vel"_json_pointer);
        for(auto&& f:parent->observables.at("/shared/fighter"_json_pointer).items()){
            std::string n=f.key();
            if(n==parent->getFullName()){
                continue;
            }
            if(idx>=maxTrackNum["Friend"]){
                break;
            }
            friendPos0.push_back(f.value().at("/motion/pos"_json_pointer));
            friendVel.push_back(f.value().at("/motion/vel"_json_pointer));
            idx++;
        }
        int numFriends=idx;
        double dt=manager->getAgentInterval()*manager->getBaseTimeStep();
        int maxT=*std::max_element(pastPoints.begin(),pastPoints.end());
        for(int t=0;t<maxT;++t){
            Eigen::VectorXd obs=Eigen::VectorXd::Zero(singleDim);
            int ofs=0;
            double delta=(t+1)*dt;
            Eigen::Vector3d vel=myVel;
            Eigen::Vector3d myPos=myPos0-vel*delta;
            Eigen::Vector3d pos=myPos;
            double V=vel.norm();
            obs.block(ofs+0,0,3,1)=pos.array()*xyInv.array()/Eigen::Array3d(std::max(dOut,dLine),std::max(dOut,dLine),hLim);
            obs(ofs+3)=V/fgtrVelNormalizer;
            obs.block(ofs+4,0,3,1)=(vel/V).array()*xyInv.array();
            obs(ofs+7)=parent->observables.at("/weapon/remMsls"_json_pointer);
            ofs+=8;
            idx=0;
            for(auto&& f:parent->observables.at("/shared/fighter"_json_pointer).items()){
                std::string n=f.key();
                if(n==parent->getFullName()){
                    continue;
                }
                if(idx>=maxTrackNum["Friend"]){
                    break;
                }
                vel=friendVel[idx];
                pos=friendPos0[idx]-vel*delta;
                MotionState m(parent->observables.at("motion"));
                Eigen::Vector3d dr=m.relPtoH(pos)-myPos;
                Eigen::Vector3d dv=m.relPtoH(vel);
                double R=dr.norm();
                V=dv.norm();
                obs(ofs+0)=R/rangeNormalizer;
                obs.block(ofs+1,0,3,1)=(dr/R).array()*xyInv.array();
                obs(ofs+4)=V/fgtrVelNormalizer;
                obs.block(ofs+5,0,3,1)=(dv/V).array()*xyInv.array();
                obs(ofs+8)=f.value().at("/weapon/remMsls"_json_pointer);
                ofs+=9;
                idx++;
            }
            pastData[t]=obs.cast<float>();
        }
    }
    int bufferSize=*std::max_element(pastPoints.begin(),pastPoints.end());
    idx=bufferSize-1-(count%bufferSize);
    Eigen::VectorXf totalObs=Eigen::VectorXf::Zero(singleDim*(1+pastPoints.size()));
    int ofs=0;
    totalObs.block(ofs+0,0,singleDim,1)=current;
    ofs+=singleDim;
    for(int frame=0;frame<pastPoints.size();++frame){
        totalObs.block(ofs+0,0,singleDim,1)=pastData[(idx+pastPoints[frame])%bufferSize];
        ofs+=singleDim;
    }
    pastData[idx]=current;
    return py::cast(totalObs);
}
Eigen::VectorXf R3AgentSample01::makeSingleObs(){
    Eigen::VectorXd obs=Eigen::VectorXd::Zero(singleDim);
    Eigen::Vector3d pos,vel,dr,dv;
    double V,R;
    int ofs=0;
    //自機
    MotionState myMotion(parent->observables.at("motion"));
    pos=myMotion.pos;
    vel=myMotion.vel;
    V=vel.norm();
    obs.block(ofs+0,0,3,1)=pos.array()*xyInv.array()/Eigen::Array3d(std::max(dOut,dLine),std::max(dOut,dLine),hLim);
    obs(ofs+3)=V/fgtrVelNormalizer;
    obs.block(ofs+4,0,3,1)=(vel/V).array()*xyInv.array();
    obs(ofs+7)=parent->observables.at("/weapon/remMsls"_json_pointer);
    ofs+=8;
    //味方機
    int idx=0;
    for(auto&& f:parent->observables.at("/shared/fighter"_json_pointer).items()){
        std::string n=f.key();
        if(n==parent->getFullName()){
            continue;
        }
        if(idx>=maxTrackNum["Friend"]){
            break;
        }
        if(f.value().at("isAlive")){
            MotionState fm(f.value().at("motion"));
            dr=myMotion.absPtoH(fm.pos);
            dv=myMotion.relPtoH(fm.vel);
            R=dr.norm();
            V=dv.norm();
            obs(ofs+0)=R/rangeNormalizer;
            obs.block(ofs+1,0,3,1)=(dr/R).array()*xyInv.array();
            obs(ofs+4)=V/fgtrVelNormalizer;
            obs.block(ofs+5,0,3,1)=(dv/V).array()*xyInv.array();
            obs(ofs+8)=f.value().at("/weapon/remMsls"_json_pointer);
            ofs+=9;
            idx++;
        }
    }
    while(idx<maxTrackNum["Friend"]){//0埋め
        ofs+=9;
        idx++;
    }
    //彼機(味方の誰かが探知しているものだけ諸元入り)
	//自分のtrackを近い方から順に読んで入れていく
    lastTrackInfo.clear();
    for(auto&& t:parent->observables.at("/sensor/track"_json_pointer)){
        lastTrackInfo.push_back(t);
    }
    std::sort(lastTrackInfo.begin(),lastTrackInfo.end(),
    [myMotion](Track3D& lhs,Track3D& rhs)->bool{
        return (lhs.posI()-myMotion.pos).norm()<(rhs.posI()-myMotion.pos).norm();
    });
    idx=0;
    for(auto&& t:lastTrackInfo){
        if(idx>=maxTrackNum["Enemy"]){
            break;
        }
        dr=myMotion.absPtoH(t.posI());
        dv=myMotion.relPtoH(t.velI());
        R=dr.norm();
        V=dv.norm();
        obs(ofs+0)=R/rangeNormalizer;
        obs.block(ofs+1,0,3,1)=(dr/R).array()*xyInv.array();
        obs(ofs+4)=V/fgtrVelNormalizer;
        obs.block(ofs+5,0,3,1)=(dv/V).array()*xyInv.array();
        ofs+=8;
        idx++;
    }
    while(idx<maxTrackNum["Enemy"]){//0埋め
        ofs+=8;
        idx++;
    }
	//味方誘導弾(射撃時刻が古いものから最大N発分)
    std::vector<nl::json> msls;
    for(auto&& msl:parent->observables.at("/weapon/missiles"_json_pointer)){
        msls.push_back(msl);
    }
    for(auto&& fr:parent->observables.at("/shared/fighter"_json_pointer).items()){
        std::string n=fr.key();
        if(n==parent->getFullName()){
            continue;
        }
        for(auto&& msl:fr.value().at("/weapon/missiles"_json_pointer)){
            msls.push_back(msl);
        }
    }
    std::sort(msls.begin(),msls.end(),
    [](const nl::json& lhs,const nl::json& rhs){
        double lhsT,rhsT;
        if(lhs.at("isAlive").get<bool>() && lhs.at("hasLaunched").get<bool>()){
            lhsT=lhs.at("launchedT").get<double>();
        }else{
            lhsT=std::numeric_limits<double>::infinity();
        }
        if(rhs.at("isAlive").get<bool>() && rhs.at("hasLaunched").get<bool>()){
            rhsT=rhs.at("launchedT").get<double>();
        }else{
            rhsT=std::numeric_limits<double>::infinity();
        }
        return lhsT<rhsT;
    });
    idx=0;
    for(auto&& m:msls){
        if(idx>=maxMissileNum["Friend"] || !(m.at("isAlive").get<bool>()&&m.at("hasLaunched").get<bool>())){
            break;
        }
        MotionState mm(m.at("motion"));
        dr=myMotion.absPtoH(mm.pos);
        dv=myMotion.relPtoH(mm.vel);
        R=dr.norm();
        V=dv.norm();
        obs(ofs+0)=R/rangeNormalizer;
        obs.block(ofs+1,0,3,1)=(dr/R).array()*xyInv.array();
        obs(ofs+4)=V/mslVelNormalizer;
        obs.block(ofs+5,0,3,1)=(dv/V).array()*xyInv.array();
        Missile::Mode mode=jsonToEnum<Missile::Mode>(m.at("mode"));
        if(mode==Missile::Mode::GUIDED){
            obs.block(ofs+8,0,3,1)=Eigen::Vector3d(1,0,0);
        }else if(mode==Missile::Mode::SELF){
            obs.block(ofs+8,0,3,1)=Eigen::Vector3d(0,1,0);
        }else{
            obs.block(ofs+8,0,3,1)=Eigen::Vector3d(0,0,1);
        }
        ofs+=11;
        idx++;
    }
    while(idx<maxMissileNum["Friend"]){//0埋め
        ofs+=11;
        idx++;
    }
	//彼誘導弾(MWSで探知したもののうち自身の正面に近いものから最大N発)
    std::vector<Track2D> mws;
    for(auto&& m:parent->observables.at("/sensor/mws/track"_json_pointer)){
        mws.push_back(m);
    }
    std::sort(mws.begin(),mws.end(),
    [myMotion](const Track2D& lhs,const Track2D& rhs)->bool{
        return -lhs.dirI().dot(myMotion.relBtoP(Eigen::Vector3d(1,0,0)))
            <-rhs.dirI().dot(myMotion.relBtoP(Eigen::Vector3d(1,0,0)));
    });
    idx=0;
    for(auto&& m:mws){
        if(idx>=maxMissileNum["Enemy"]){
            break;
        }
        obs.block(ofs+0,0,3,1)=m.dirI().array()*xyInv.array();
        ofs+=3;
        idx++;
    }
    while(idx<maxMissileNum["Enemy"]){//0埋め
        ofs+=3;
        idx++;
    }
    return obs.cast<float>();
}
py::object R3AgentSample01::action_space(){
    turnTable=Eigen::VectorXd::LinSpaced(turnDim,-turnScale,turnScale);
    pitchTable=Eigen::VectorXd::LinSpaced(pitchDim,-pitchScale,pitchScale);
    accelTable=Eigen::VectorXd::LinSpaced(accelDim,-accelScale,accelScale);
    if(turnDim%2!=0){turnTable(turnDim/2)=0.0;}//force center value strictly to be zero
    if(pitchDim%2!=0){pitchTable(pitchDim/2)=0.0;}//force center value strictly to be zero
    if(accelDim%2!=0){accelTable(accelDim/2)=0.0;}//force center value strictly to be zero
    fireTable=Eigen::VectorXd::Zero(maxTrackNum["Enemy"]+1);
    for(int i=0;i<maxTrackNum["Enemy"]+1;++i){
        fireTable(i)=i-1;
    }
    VecX<long> nvec=VecX<long>::Zero(4);
    nvec<<turnTable.rows(),pitchTable.rows(),accelTable.rows(),fireTable.rows();
    py::module_ spaces=py::module_::import("gym.spaces");
    return spaces.attr("MultiDiscrete")(nvec);
}
void R3AgentSample01::deploy(py::object action_){
    auto action=py::cast<VecX<long>>(action_);
    MotionState myMotion(parent->observables.at("motion"));
    double pAZ=myMotion.az;
    double turn=turnTable(action(0));
    double pitch=pitchTable(action(1));
    dstDir<<cos(pAZ+turn)*cos(pitch),sin(pAZ+turn)*cos(pitch),sin(pitch);
    if(!(accelTable(lastAction(2))==0 && accelTable(action(2))==0)){
        baseV=myMotion.vel.norm();
    }
    dstV=baseV+accelTable(action(2));
    if(baseV<minimumV){
        velRecovery=true;
    }
    if(baseV>=minimumRecoveryV){
        velRecovery=false;
    }
    if(velRecovery){
        dstV=minimumRecoveryDstV;
    }
    int shoot=int(action(3))-1;
    int flyingMsls=0;
    for(auto&& msl:parent->observables.at("/weapon/missiles"_json_pointer)){
        if(msl.at("isAlive").get<bool>() && msl.at("hasLaunched").get<bool>()){
            flyingMsls++;
        }
    }
    if(shoot>=lastTrackInfo.size() || flyingMsls>=maxSimulShot){
        shoot=-1;
    }
    if(shoot>=0){
        launchFlag=true;
        target=lastTrackInfo[shoot];
    }else{
        launchFlag=false;
        target=Track3D();
    }
    observables[parent->getFullName()]["decision"]={
        {"Roll",nl::json::array({"Don't care"})},
        {"Horizontal",nl::json::array({"Az_BODY",turn})},
        {"Vertical",nl::json::array({"El",-pitch})},
        {"Throttle",nl::json::array({"Vel",dstV})},
        {"Fire",nl::json::array({launchFlag,target})}
    };
    lastAction=action;
}
void R3AgentSample01::control(){
    MotionState myMotion(parent->observables.at("motion"));
    Eigen::Vector3d pos=myMotion.pos;
    Eigen::Vector3d vel=myMotion.vel;
    if(abs(pos(0))>=dOutLimitRatio*dOut){
		//戦域逸脱を避けるための方位補正
		//判定ラインの超過具合に応じて復帰角度を変化させる。(無限遠でラインに直交、ライン上でラインと平行)
        double over=abs(pos(0))/dOut-dOutLimitRatio;
		double n=sqrt(dstDir(0)*dstDir(0)+dstDir(1)*dstDir(1));
		double theta=atan(over);
		double cs=cos(theta);
		double sn=sin(theta);
		if(pos(0)>0){//北側
			if(dstDir(1)>0){//東向き
				if(atan2(-dstDir(0),dstDir(1))<theta){
				    dstDir=Eigen::Vector3d(-n*sn,n*cs,dstDir(2));
                }
            }else{//西向き
				if(atan2(-dstDir(0),-dstDir(1))<theta){
					dstDir=Eigen::Vector3d(-n*sn,-n*cs,dstDir(2));
                }
            }
        }else{//南側
			if(dstDir(1)>0){//東向き
				if(atan2(dstDir(0),dstDir(1))<theta){
				    dstDir=Eigen::Vector3d(n*sn,n*cs,dstDir(2));
                }
            }else{//西向き
				if(atan2(dstDir(0),-dstDir(1))<theta){
					dstDir=Eigen::Vector3d(n*sn,-n*cs,dstDir(2));
                }
            }
        }
    }
	if(-pos(2)<hMin){
		//高度下限を下回った場合
		double over=hMin+pos(2);
		double n=sqrt(dstDir(0)*dstDir(0)+dstDir(1)*dstDir(1));
		double theta=atan(over);
		double cs=cos(theta);
		double sn=sin(theta);
		dstDir=Eigen::Vector3d(dstDir(0)/n*cs,dstDir(1)/n*cs,-sn);
    }else if(-pos(2)>hMax){
		//高度上限を上回った場合
		double over=-pos(2)-hMax;
		double n=sqrt(dstDir(0)*dstDir(0)+dstDir(1)*dstDir(1));
		double theta=atan(over);
		double cs=cos(theta);
		double sn=sin(theta);
		dstDir=Eigen::Vector3d(dstDir(0)/n*cs,dstDir(1)/n*cs,sn);
    }
    if(parent->isinstance<CoordinatedFighter>()){
        commands[parent->getFullName()]={
            {"motion",{
                {"dstDir",dstDir},
                {"dstV",dstV}
            }},
            {"weapon",{
                {"launch",launchFlag},
                {"target",target}
            }}
        };
    }else if(parent->isinstance<MassPointFighter>()){
        double V=vel.norm();
        Eigen::Vector3d Vn=vel/V;
        double cs=Vn.dot(dstDir);
        Eigen::Vector3d delta=Vn.cross(dstDir);
        double sn=delta.norm();
        double theta=atan2(sn,cs);
        Eigen::Vector3d omegaI,omegaB;
        if(theta<1e-6){
            omegaI<<0,0,0;
        }else if(theta>M_PI-1e-6){
            Eigen::Vector3d pz=(Vn.cross(Eigen::Vector3d(0,0,1).cross(Vn))).normalized();
            omegaI=pz*M_PI;
        }else{
            omegaI=delta*(theta/sn);
        }
        omegaB=myMotion.relPtoB(omegaI);
        commands[parent->getFullName()]={
            {"motion",{
                {"roll",omegaB(0)*omegaScale(0)},
                {"pitch",omegaB(1)*omegaScale(1)},
                {"yaw",omegaB(2)*omegaScale(2)},
                {"throttle",std::clamp((dstV-V)/accelScale*0.5+0.5,-1.0,1.0)}
            }},
            {"weapon",{
                {"launch",launchFlag},
                {"target",target}
            }}
        };
    }
}
py::object R3AgentSample01::convertActionFromAnother(const nl::json& decision,const nl::json& command){
    double interval=manager->getAgentInterval()*manager->getBaseTimeStep();
    MotionState myMotion(parent->observables.at("motion"));
	//ロールは無視
    //水平方向
    int turnIdx,pitchIdx,accelIdx,fireIdx;
    std::string type=decision[parent->getFullName()]["Horizontal"][0];
    double value=decision[parent->getFullName()]["Horizontal"][1];
    double dAZ=0.0;
	if(type=="Rate"){
        dAZ=value*interval;
    }else if(type=="Az_NED"){
		dAZ=value-myMotion.az;
    }else if(type=="Az_BODY"){
        dAZ=value;
    }
    Eigen::VectorXd tmp=turnTable-Eigen::VectorXd::Constant(turnTable.rows(),atan2(sin(dAZ),cos(dAZ)));
    tmp.cwiseAbs().minCoeff(&turnIdx);
	//垂直方向
    type=decision[parent->getFullName()]["Vertical"][0];
    value=decision[parent->getFullName()]["Vertical"][1];
    double el=0.0;
	if(type=="Rate"){
        el=value*interval;
    }else if(type=="El"){
		el=value;
    }else if(type=="Pos"){
        double tau=10.0;
		el=std::max(-pitchScale,std::min(pitchScale,deg2rad((value-myMotion.pos(2))/tau)));
    }
    tmp=pitchTable-Eigen::VectorXd::Constant(pitchTable.rows(),atan2(sin(el),cos(el)));
    tmp.cwiseAbs().minCoeff(&pitchIdx);
	//加減速
    type=decision[parent->getFullName()]["Throttle"][0];
    value=decision[parent->getFullName()]["Throttle"][1];
    baseV=myMotion.vel.norm();
    if(parent->isinstance<CoordinatedFighter>()){
        //decisionよりcommandを優先する
        nl::json m=command[parent->getFullName()].at("motion");
        if(m.contains("dstV")){
            dstV=m.at("dstV");
        }else{
            if(type=="Vel"){
	        	dstV=value;
            }else if(type=="Throttle"){
                //0〜1のスロットルで指定していた場合は、0と1をそれぞれ加減速テーブルの両端とみなし線形変換する。
                dstV=baseV+accelScale*(2*value-1);
            }else{//type=="Accel"){
                //加速度ベースの指定だった場合は、機体性能や旋回状況に依存するうえ飛行制御則によっても変わり、正確な変換は難しいため符号が合っていればよいという程度で変換。
                dstV=baseV+std::min(accelScale,std::max(-accelScale,value*15));
            }
        }
    }else if(parent->isinstance<MassPointFighter>()){
	    if(type=="Vel"){
    		dstV=value;
        }else if(type=="Throttle"){
            //0〜1のスロットルで指定していた場合は、0と1をそれぞれ加減速テーブルの両端とみなし線形変換する。
            dstV=baseV+accelScale*(2*value-1);
        }else{//type=="Accel"){
            //加速度ベースの指定だった場合は、機体性能や旋回状況に依存するうえ飛行制御則によっても変わり、正確な変換は難しいため符号が合っていればよいという程度で変換。
            dstV=baseV+std::min(accelScale,std::max(-accelScale,value*15));
        }
    }
    tmp=accelTable-Eigen::VectorXd::Constant(accelTable.rows(),dstV-baseV);
    tmp.cwiseAbs().minCoeff(&accelIdx);
	//射撃
	if(decision[parent->getFullName()]["Fire"][0].get<bool>()){
		Track3D expertTarget=decision[parent->getFullName()]["Fire"][1];
        fireIdx=0;
        int trackIdx=0;
        for(auto&& t:lastTrackInfo){
            if(t.isSame(expertTarget)){
                fireIdx=trackIdx+1;
            }
            trackIdx++;
        }
        if(fireIdx>=fireTable.size()){
            fireIdx=0;
        }
    }else{
        fireIdx=0;
    }
    VecX<long> ret=VecX<long>::Zero(4);
    ret<<turnIdx,pitchIdx,accelIdx,fireIdx;
    return py::cast(ret);
}

void exportR3AgentSample01(py::module &m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(R3AgentSample01)
    DEF_FUNC(R3AgentSample01,validate)
    DEF_FUNC(R3AgentSample01,observation_space)
    DEF_FUNC(R3AgentSample01,makeObs)
    DEF_FUNC(R3AgentSample01,action_space)
    DEF_FUNC(R3AgentSample01,deploy)
    DEF_FUNC(R3AgentSample01,control)
    DEF_FUNC(R3AgentSample01,convertActionFromAnother)
    .def("convertActionFromAnother",[](R3AgentSample01& v,const py::object& decision,const py::object& command){
        return v.convertActionFromAnother(decision,command);
    })
    ;
}

