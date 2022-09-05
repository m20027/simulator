#include "R3InitialFighterAgent01.h"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "SimulationManager.h"
#include "Fighter.h"
#include "MassPointFighter.h"
#include "CoordinatedFighter.h"
#include "Missile.h"
#include "Ruler.h"
#include "Units.h"
using namespace util;
TrackInfo::TrackInfo():Track3D(){
    idx=-1;
    distance=myRHead=myRTail=hisRHead=hisRTail=0.0;
    inOurSensor=SensingState::OUTSIDE;inMySensor=SensingState::OUTSIDE;
    numTracker=numTrackerLimit=0;
    trackers.clear();
    limitTrackers.clear();
    nonLimitTrackers.clear();
    state=UpdateState::LOST;
    memoryStartTime=0.0;
}
TrackInfo::TrackInfo(const Track3D& original_,int idx_)
:Track3D(original_){
    idx=idx_;
    distance=myRHead=myRTail=hisRHead=hisRTail=0.0;
    inOurSensor=SensingState::INSIDE;inMySensor=SensingState::INSIDE;
    numTracker=numTrackerLimit=0;
    trackers.clear();
    limitTrackers.clear();
    nonLimitTrackers.clear();
    state=UpdateState::TRACK;
    memoryStartTime=0.0;
}
TrackInfo::TrackInfo(const nl::json& j_)
:TrackInfo(j_.get<TrackInfo>()){
}
TrackInfo::TrackInfo(const TrackInfo &other):Track3D(other){
    update(other,true);
}
TrackInfo::~TrackInfo(){}
TrackInfo TrackInfo::copy() const{
    return TrackInfo(*this);
}
void TrackInfo::update(const TrackInfo& other,bool isFull){
    Track3D::update(other);
	distance=other.distance;
	if(isFull){
        idx=other.idx;
		myRHead=other.myRHead;
		myRTail=other.myRTail;
		hisRHead=other.hisRHead;
		hisRTail=other.hisRTail;
		inOurSensor=other.inOurSensor;
		inMySensor=other.inMySensor;
		numTracker=other.numTracker;
		numTrackerLimit=other.numTrackerLimit;
        trackers.clear();
        for(auto & e:other.trackers){
            trackers.push_back(e);
        }
        limitTrackers.clear();
        for(auto & e:other.limitTrackers){
            limitTrackers.push_back(e);
        }
        nonLimitTrackers.clear();
        for(auto & e:other.nonLimitTrackers){
            nonLimitTrackers.push_back(e);
        }
		state=other.state;
		memoryStartTime=other.memoryStartTime;
    }
}
nl::json TrackInfo::to_json() const{
    return *this;
}
void to_json(nlohmann::json& nlohmann_json_j, const TrackInfo& nlohmann_json_t){
    to_json(nlohmann_json_j,(const Track3D&)nlohmann_json_t);
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO,idx,distance,myRHead,myRTail,hisRHead,hisRTail,numTracker,numTrackerLimit,trackers,limitTrackers,nonLimitTrackers,memoryStartTime))
    nlohmann_json_j["inOurSensor"]=enumToJson(nlohmann_json_t.inOurSensor);
    nlohmann_json_j["inMySensor"]=enumToJson(nlohmann_json_t.inMySensor);
    nlohmann_json_j["state"]=enumToJson(nlohmann_json_t.state);
}
void from_json(const nlohmann::json& nlohmann_json_j, TrackInfo& nlohmann_json_t){
    from_json(nlohmann_json_j,(Track3D&)nlohmann_json_t);
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM,idx,distance,myRHead,myRTail,hisRHead,hisRTail,numTracker,numTrackerLimit,trackers,limitTrackers,nonLimitTrackers,memoryStartTime))
    nlohmann_json_t.inOurSensor=jsonToEnum<TrackInfo::SensingState>(nlohmann_json_j.at("inOurSensor"));
    nlohmann_json_t.inMySensor=jsonToEnum<TrackInfo::SensingState>(nlohmann_json_j.at("inMySensor"));
    nlohmann_json_t.state=jsonToEnum<TrackInfo::UpdateState>(nlohmann_json_j.at("state"));
}

R3InitialFighterAgent01::R3InitialFighterAgent01(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:SingleAssetAgent(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    //失探後の航跡の保持に関するパラメータ
    tMaxMemory=getValueFromJsonKRD(modelConfig,"tMaxMemory",randomGen,10);
    //探知状況の分類に関するパラメータ
    sensorInRangeLimit=getValueFromJsonKRD(modelConfig,"sensorInRangeLimit",randomGen,10000);
    sensorInCoverageLimit=deg2rad(getValueFromJsonKRD(modelConfig,"sensorInCoverageLimit",randomGen,5));
    //(c1)射撃条件に関するパラメータ
    kShoot=getValueFromJsonKRD(modelConfig,"kShoot",randomGen,0.6);
    nMslSimul=getValueFromJsonKRD(modelConfig,"nMslSimul",randomGen,1);
    //(c2)離脱条件に関するパラメータ
    kBreak=getValueFromJsonKRD(modelConfig,"kBreak",randomGen,0.25);
    //(c3)離脱終了条件に関するパラメータ
    tWithdraw=getValueFromJsonKRD(modelConfig,"tWithdraw",randomGen,50);
    //(s1)通常時の行動選択に関するパラメータ
    pAdvanceAlly=getValueFromJsonKRD(modelConfig,"pAdvanceAlly",randomGen,60);
    pApproachAlly=getValueFromJsonKRD(modelConfig,"pApproachAlly",randomGen,30);
    pKeepSensingAlly=getValueFromJsonKRD(modelConfig,"pKeepSensingAlly",randomGen,10);
    pApproachMyself=getValueFromJsonKRD(modelConfig,"pApproachMyself",randomGen,30);
    pKeepSensingMyself=getValueFromJsonKRD(modelConfig,"pKeepSensingMyself",randomGen,70);
    //(s1-1)前進に関するパラメータ
    dPrioritizedAdvance=getValueFromJsonKRD(modelConfig,"dPrioritizedAdvance",randomGen,0.1);
    thetaPrioritizedAdvance=deg2rad(getValueFromJsonKRD(modelConfig,"thetaPrioritizedAdvance",randomGen,45));
    //(s1-3)横行に関するパラメータ
    thetaKeepSensing=deg2rad(getValueFromJsonKRD(modelConfig,"thetaKeepSensing",randomGen,10));
    //tKeepSensingDelay=getValueFromJsonKR(modelConfig,"tKeepSensingDelay",randomGen);
    //(s3)回避に関するパラメータ
    thetaEvasion=deg2rad(getValueFromJsonKRD(modelConfig,"thetaEvasion",randomGen,-45));
    hEvasion=getValueFromJsonKRD(modelConfig,"hEvasion",randomGen,2000);
    //(o1)副目標の追尾に関するパラメータ
    thetaModForSensing=deg2rad(getValueFromJsonKRD(modelConfig,"thetaModForSensing",randomGen,10));
    //(o2)高度維持に関するパラメータ
    thetaStable=deg2rad(getValueFromJsonKRD(modelConfig,"thetaStable",randomGen,15));
    hNormal=getValueFromJsonKRD(modelConfig,"hNormal",randomGen,10000);
    //(o3)場外の防止に関するパラメータ
    dOutLimit=getValueFromJsonKRD(modelConfig,"dOutLimit",randomGen,100000);
    dOutLimitTurnAxis=getValueFromJsonKRD(modelConfig,"dOutLimitTurnAxis",randomGen,100000);
    dOutLimitKeepSensing=getValueFromJsonKRD(modelConfig,"dOutLimitKeepSensing",randomGen,200000);
    //目標選択に関するパラメータ(突破優先に関するもの)
    dPrioritizedAimLeaker=getValueFromJsonKRD(modelConfig,"dPrioritizedAimLeaker",randomGen,0.1);
    thetaPrioritizedAimLeaker=deg2rad(getValueFromJsonKRD(modelConfig,"thetaPrioritizedAimLeaker",randomGen,45));
    //最低速度維持のためのパラメータ
    minimumV=getValueFromJsonKRD(modelConfig,"minimumV",randomGen,200.0);
    minimumRecoveryV=getValueFromJsonKRD(modelConfig,"minimumRecoveryV",randomGen,220.0);
    minimumRecoveryDstV=getValueFromJsonKRD(modelConfig,"minimumRecoveryDstV",randomGen,250.0);
    nominalAccelCmd=getValueFromJsonKRD(modelConfig,"nominalAccelCmd",randomGen,30.0);
    state=State::ADVANCE;
    track.clear();
    target=TrackInfo();
    additionalTargets.clear();
    isAssignedAsInterceptor=false;
    launchFlag=false;
    launchedTime=0.0;
    launchedTarget=TrackInfo();
    withdrawnTime=0.0;
    estEnemyNoDetectionTime=-1.0;
    waitingTransition=false;
    transitionDelay=0.0;
    transitionTriggeredTime=0.0;
    transitionBefore=State::NONE;
    transitionAfter=State::NONE;
    transitionCause=TransitionCause::NONE;
    velRecovery=false;
    isAfterDeploy=false;
}
R3InitialFighterAgent01::~R3InitialFighterAgent01(){}

void R3InitialFighterAgent01::validate(){
    auto rulerObs=manager->getRuler()->observables;
    std::string eastSider=rulerObs.at("eastSider");
    if(getTeam()==eastSider){
        forward<<0,-1,0;
        rightSide<<1,0,0;
    }else{
        forward<<0,1,0;
        rightSide<<-1,0,0;
    }
    dOut=rulerObs.at("dOut");
    dLine=rulerObs.at("dLine");
    fgtrID=randomGen()%2;
    if(parent->isinstance<CoordinatedFighter>()){
        std::dynamic_pointer_cast<FighterAccessor>(parent)->setFlightControllerMode("fromDirAndVel");
    }else if(parent->isinstance<MassPointFighter>()){
        const nl::json& fSpec=parent->observables.at("/spec/dynamics"_json_pointer);
        omegaScale<<1./fSpec.at("rollMax").get<double>(),1./fSpec.at("pitchMax").get<double>(),1./fSpec.at("yawMax").get<double>();
    }else{
        throw std::runtime_error("R3InitialFighterAgent01 accepts only MassPointFighrer or CoordinatedFighter.");
    }
}
void R3InitialFighterAgent01::perceive(bool inReset){
    observables.merge_patch({{parent->getFullName(),{
        {"state",enumToJson(state)},
        {"target",target},
        {"launchedTarget",launchedTarget}
    }}});
}
void R3InitialFighterAgent01::updateMyInfo(){
    pos=parent->observables.at("/motion/pos"_json_pointer);
    vel=parent->observables.at("/motion/vel"_json_pointer);
}
void R3InitialFighterAgent01::updateTracks(bool isFull){
    track.clear();
    int idx=0;
    for(auto&& t:parent->observables.at("/sensor/track"_json_pointer)){
        track.push_back(TrackInfo(t.get<Track3D>(),idx));
        updateTrackInfo(track[idx],isFull);
        idx++;
    }
}
void R3InitialFighterAgent01::updateTrackInfo(TrackInfo& ti,bool isFull){
	/*TrackInfoの付帯情報の更新を行う。
	* 射程情報: 双方の距離,RmaxHead,Rmax1,Rmax2
	* 追尾状況: 自機が追尾中か否か,誰かが追尾中か否か,追尾機数(IN,LIMIT,OUT) ただし、stateがEVADEまたはWITHDRAWの機体はINでもLIMIT扱いとする。
	* isFullにTrueを入れたときはdeploy用に射程計算等を含む全てを計算し、Falseを入れたときは位置と速度だけ更新する。
	*/
    Eigen::Vector3d tpos=ti.posI();
    Eigen::Vector3d tvel=ti.velI();
    Eigen::Vector3d dpos=pos-tpos;
    double L=dpos.norm();
    ti.distance=L;
    if(isFull && ti.state!=TrackInfo::UpdateState::LOST){
        ti.myRHead=calcRHead(pos,vel,tpos,tvel);
        ti.myRTail=calcRTail(pos,vel,tpos,tvel);
        ti.hisRHead=calcRHead(tpos,tvel,pos,vel);
        ti.hisRTail=calcRTail(tpos,tvel,pos,vel);
        Eigen::Vector3d tEx=tvel.normalized();
        if(ti.state==TrackInfo::UpdateState::TRACK){
            ti.trackers.clear();
            for(auto& src:parent->observables.at("/sensor/trackSource"_json_pointer)[ti.idx]){
                ti.trackers.push_back(src);
            }
            ti.numTracker=ti.trackers.size();
            ti.nonLimitTrackers.clear();
            ti.limitTrackers.clear();
            ti.numTrackerLimit=0;
            ti.inOurSensor=TrackInfo::SensingState::OUTSIDE;
            ti.inMySensor=TrackInfo::SensingState::OUTSIDE;
            for(auto& e:ti.trackers){
                const nl::json& fObs=e==parent->getFullName() ?
                    parent->observables :
                    parent->observables.at("/shared/fighter"_json_pointer).at(e);
                const nl::json& aObs=parent->observables.at("/shared/agent"_json_pointer).at(e);
                Eigen::Vector3d tposS=MotionState(fObs.at("motion")).absPtoB(tpos);
                double L=tposS.norm();
                double angle=acos(std::max(-1.0,std::min(1.0,tposS(0)/L)));
                double fLref=fObs.at("/spec/sensor/radar/Lref"_json_pointer);
                double fAngle=fObs.at("/spec/sensor/radar/thetaFOR"_json_pointer);
                if(
                    L<=fLref-sensorInRangeLimit &&
                    angle<=fAngle-sensorInCoverageLimit &&
                    jsonToEnum<State>(aObs.at("state").get<std::string>())!=State::EVADE &&
                    jsonToEnum<State>(aObs.at("state").get<std::string>())!=State::WITHDRAW
                ){
                    ti.inOurSensor=TrackInfo::SensingState::INSIDE;
                    ti.nonLimitTrackers.push_back(e);
                    if(e==parent->getFullName()){
                        ti.inMySensor=TrackInfo::SensingState::INSIDE;
                    }
                }else if(L<=fLref && angle<=fAngle){
                    ti.limitTrackers.push_back(e);
                    ti.numTrackerLimit++;
                    if(ti.inOurSensor!=TrackInfo::SensingState::INSIDE){
                        ti.inOurSensor=TrackInfo::SensingState::LIMIT;
                    }
                    if(e==parent->getFullName()){
                        if(ti.inMySensor!=TrackInfo::SensingState::INSIDE){
                            ti.inMySensor=TrackInfo::SensingState::LIMIT;
                        }
                    }
                }
            }
        }else{
            ti.trackers.clear();
            ti.nonLimitTrackers.clear();
            ti.limitTrackers.clear();
            ti.numTracker=0;
            ti.numTrackerLimit=0;
            ti.inOurSensor=TrackInfo::SensingState::OUTSIDE;
            ti.inMySensor=TrackInfo::SensingState::OUTSIDE;
        }
    }
}
void R3InitialFighterAgent01::updateTargetStatus(bool isFull){
	/*主目標と副目標を更新する。副目標は、前回の各機の主目標と、まだ誘導が必要な全ての誘導弾目標とし、副目標内でのtruthの重複はないようにする。
	「誘導が必要」とは、誘導弾自身のセンサで捉えていない状態(mode!=SELF)を指すものとする。
	なお、副目標には自機の主目標も含む(敢えて除外していない)
	isFullにTrueを入れたときは副目標の再抽出も行い全て更新するが、Falseを入れたときはその時点で抽出されているものの位置と速度だけ更新する。
	*/
	//主目標の更新
    updateTargetStatusSub(target,isFull);
    updateTargetStatusSub(launchedTarget,isFull);
    //副目標の確定(isFullの時のみ)
    if(isFull){
        std::vector<bool> flag;//いらないものを削除するためのフラグ
        for(auto& t:additionalTargets){
            flag.push_back(false);
        }
        for(auto&& fr:parent->observables.at("/shared/fighter"_json_pointer).items()){
            if(fr.value().at("isAlive")){
                const nl::json& ag=parent->observables.at("/shared/agent"_json_pointer).at(fr.key());
                //主目標
                TrackInfo tgt=TrackInfo(ag.at("target"),-1);
                if(!tgt.is_none()){
                    bool ret=true;
                    int idx=0;
                    for(auto& aTgt:additionalTargets){
                        if(aTgt.isSame(tgt)){
                            ret=false;
                            flag[idx]=true;
                            break;
                        }
                        idx++;
                    }
                    if(ret){
                        additionalTargets.push_back(tgt);
                        flag.push_back(true);
                    }
                }
                //直近の射撃目標
                tgt=TrackInfo(ag.at("launchedTarget"),-1);
                if(!tgt.is_none()){
                    int nextMsl=fr.value().at("/weapon/nextMsl"_json_pointer);
                    if(nextMsl>0){
                        const nl::json& msl=fr.value().at("/weapon/missiles"_json_pointer)[nextMsl-1];
                        if(msl.at("isAlive").get<bool>() && jsonToEnum<Missile::Mode>(msl.at("mode"))!=Missile::Mode::SELF){//まだ誘導が必要
                            bool ret=true;
                            int idx=0;
                            for(auto& aTgt:additionalTargets){
                                if(aTgt.isSame(tgt)){
                                    ret=false;
                                    flag[idx]=true;
                                    break;
                                }
                                idx++;
                            }
                            if(ret){
                                additionalTargets.push_back(tgt);
                                flag.push_back(true);
                            }
                        }
                    }
                }
            }
        }
        //まだ誘導が必要な誘導弾の目標は直近のでなくても残す
        for(auto&& msl:parent->observables.at("/weapon/missiles"_json_pointer)){
            if(msl.at("isAlive").get<bool>() && msl.at("hasLaunched").get<bool>() && jsonToEnum<Missile::Mode>(msl.at("mode"))!=Missile::Mode::SELF){
                int idx=0;
                for(auto& aTgt:additionalTargets){
                    if(msl.at("target").get<Track3D>().isSame(aTgt)){
                        flag[idx]=true;
                            idx++;
                    }
                }
            }
        }
        for(auto&& fr:parent->observables.at("/shared/fighter"_json_pointer).items()){
            if(fr.key()!=parent->getFullName()){
                for(auto&& msl:fr.value().at("/weapon/missiles"_json_pointer)){
                    if(msl.at("isAlive").get<bool>() && msl.at("hasLaunched").get<bool>() && jsonToEnum<Missile::Mode>(msl.at("mode"))!=Missile::Mode::SELF){
                        int idx=0;
                        for(auto& aTgt:additionalTargets){
                            if(msl.at("target").get<Track3D>().isSame(aTgt)){
                                flag[idx]=true;
                                    idx++;
                            }
                        }
                    }
                }
            }
        }
        auto ait=additionalTargets.begin();
        for(auto fit=flag.begin();fit!=flag.end();){
            if((*fit) && (*ait).state!=TrackInfo::UpdateState::LOST){
                ++fit;
                ++ait;
            }else{
                fit=flag.erase(fit);
                ait=additionalTargets.erase(ait);
            }
        }
    }
    //副目標の更新
    for(auto& aTgt:additionalTargets){
        updateTargetStatusSub(aTgt,isFull);
    }
    //行動判断の考慮対象(航跡＋メモリトラック中の主or副目標)の抽出
    allEnemies.clear();
    for(auto& t:track){
        allEnemies.push_back(t);
    }
    for(auto& t:additionalTargets){
        if(t.state==TrackInfo::UpdateState::MEMORY){
            allEnemies.push_back(t);
        }
    }
}
void R3InitialFighterAgent01::updateTargetStatusSub(TrackInfo& tgt,bool isFull){
    /*指定した目標情報を更新する。
    */
    if(!tgt.is_none()){
        bool updated=false;
        for(auto& t:track){//航跡情報から更新対象を検索
            if(tgt.isSame(t)){
                tgt.update(t,isFull);
                updated=true;
                break;
            }
        }
        if(!updated){//見つからなかった場合、メモリトラック
            if(tgt.state==TrackInfo::UpdateState::TRACK){
                tgt.memoryStartTime=getT();
                tgt.state=TrackInfo::UpdateState::MEMORY;
            }else if(tgt.state==TrackInfo::UpdateState::MEMORY){
                if(getT()>=tMaxMemory+tgt.memoryStartTime){
                    tgt.state=TrackInfo::UpdateState::LOST;
                }
            }
        }
        if(tgt.state!=TrackInfo::UpdateState::TRACK){//外挿
            tgt.updateByExtrapolation(manager->getBaseTimeStep());
            updateTrackInfo(tgt,isFull);
        }
    }
}
bool R3InitialFighterAgent01::chkMWS(){
    return parent->observables.at("/sensor/mws/track"_json_pointer).get<std::vector<Track2D>>().size()>0;
}
bool R3InitialFighterAgent01::chkLaunchable(){
    //自機が射撃可能な状態かどうかを返す。
    const nl::json& fObs=parent->observables;
    return fObs.at("/weapon/remMsls"_json_pointer)>0 && fObs.at("/weapon/launchable"_json_pointer);
}
std::pair<bool,std::pair<TrackInfo,double>> R3InitialFighterAgent01::chkConditionForShoot(TrackInfo& tgt){
	/*(c1)射撃条件の判定
    1. 射程による判定・・・その航跡に対してRtailを0、Rheadを1としたときに現在の距離がkShoot以下であること
    2. 探知状況による判定・・・その航跡を自身か味方の少なくとも１機が余裕をもって捉えていること
    3. 射撃状況による判定・・・その航跡に対して自身が発射した飛翔中の誘導弾がnMslSimul発未満であること
	*/
    //条件1：射程による判定
    bool ret1=!tgt.is_none();
    double r;
    if(ret1){
        r=tgt.distance-tgt.myRTail;
        double delta=tgt.myRHead-tgt.myRTail;
        if(delta==0){
            if(r<0){
                r=-std::numeric_limits<double>::infinity();
            }else if(r>0){
                r=std::numeric_limits<double>::infinity();
            }else{
                r=0;
            }
        }else{
            r/=delta;
        }
        ret1=r<=kShoot;
    }else{
        r=0;
    }
    //条件2：探知状況による判定
    bool ret2=false;
    for(auto& f:tgt.nonLimitTrackers){
        auto s=jsonToEnum<State>(parent->observables.at("/shared/agent"_json_pointer).at(f).at("state"));
        if(s!=State::EVADE && s!=State::WITHDRAW){
            ret2=true;
            break;
        }
    }
    //条件3:射撃状況による判定
    bool ret3=true;
    int count=0;
    for(auto&& msl:parent->observables.at("/weapon/missiles"_json_pointer)){
        if(msl.at("isAlive").get<bool>() && msl.at("hasLaunched").get<bool>() && jsonToEnum<Missile::Mode>(msl.at("mode"))!=Missile::Mode::SELF){
            if(msl.at("target").get<Track3D>().isSame(tgt)){
                count++;
                if(count>=nMslSimul){
                    ret3=false;
                    break;
                }
            }
        }
    }
    return std::make_pair(ret1 && ret2 && ret3,std::make_pair(tgt,r));
}
std::vector<bool> R3InitialFighterAgent01::chkBreak(){
    //(c2)離脱条件の判定
    std::vector<bool> ret;
    for(auto& t:allEnemies){
        ret.push_back((t.distance-t.hisRTail)<=(t.hisRHead-t.hisRTail)*kBreak);
    }
    return ret;
}
bool R3InitialFighterAgent01::chkTransitionCount(){
    return waitingTransition && getT()-transitionTriggeredTime>=transitionDelay;
}
void R3InitialFighterAgent01::reserveTransition(const State& dst,double delay){
    transitionTriggeredTime=getT();
    transitionBefore=state;
    transitionAfter=dst;
    transitionDelay=delay;
    waitingTransition=true;
}
void R3InitialFighterAgent01::completeTransition(){
    bool isAbnormal=(
        (transitionAfter!=State::ADVANCE && transitionAfter!=State::APPROACH_TARGET && transitionAfter!=State::KEEP_SENSING)
    );
    if(isAbnormal){
        transitionCause=TransitionCause::ABNORMAL;
    }
    waitingTransition=false;
    state=transitionAfter;
}
void R3InitialFighterAgent01::cancelTransition(){
    waitingTransition=false;
}
void R3InitialFighterAgent01::immidiateTransition(const State& dst){
    bool isAbnormal=(
        (dst!=State::ADVANCE && dst!=State::APPROACH_TARGET && dst!=State::KEEP_SENSING)
    );
    if(isAbnormal){
        transitionCause=TransitionCause::ABNORMAL;
        //std::cout<<"abnormal, dst="<<dst<<std::endl;
    }
    waitingTransition=false;
    state=dst;
}
void R3InitialFighterAgent01::selectTarget(){
	/*主目標の選択を行う。候補は全航跡に加え、メモリトラック中の副目標(主目標も含む)とする
	1. 自陣に十分近いか、敵陣側扇範囲内に捉えている味方がいないのいずれを満たす敵がいれば最も近い味方を割り振る。
	パラメータはdPrioritizedAimLeaker,thetaPrioritizedAimLeakerの二つ
	2. 距離の近い順に割り振る。
	*/
    //まず、lostしてしまったものを削除
    if(!target.is_none() && target.state==TrackInfo::UpdateState::LOST){
        target=TrackInfo();
    }
    if(!launchedTarget.is_none() && launchedTarget.state==TrackInfo::UpdateState::LOST){
        launchedTarget=TrackInfo();
    }
    std::vector<Eigen::Vector3d> fpos,fvel;
    std::vector<bool> assigned;
    assigned.push_back(false);
    fpos.push_back(pos);
    fvel.push_back(vel);
    for(auto&& f:parent->observables.at("/shared/fighter"_json_pointer).items()){
        assigned.push_back(false);
        MotionState mo(f.value().at("motion"));
        fpos.push_back(mo.pos);
        fvel.push_back(mo.vel);
    }
    std::size_t numF=fpos.size();
    std::vector<TrackInfo*> candidates;
    std::vector<Eigen::Vector3d> tpos;
    for(auto& t:allEnemies){
        if(t.state!=TrackInfo::UpdateState::LOST){
            candidates.push_back(&t);
            tpos.push_back(t.posI());
        }
    }
    isAssignedAsInterceptor=false;
    if(candidates.size()==0){//何も見えていない場合
        target=TrackInfo();
        return;
    }
	//(1)突破阻止(複数該当した場合は、とりあえずラインに近い敵から順に、最も近い未割当の味方を割り当てる)
    std::vector<std::pair<double,int>> leakers,nearestF;
    Eigen::MatrixXd dist=Eigen::MatrixXd::Zero(fpos.size(),tpos.size());
    int i=0;int j=0;
    for(auto& tp:tpos){
        double distFromLine=dLine-tp.dot(-forward);
        bool isLeaker=distFromLine<dPrioritizedAimLeaker*dLine;
        double angle=M_PI;
        i=0;
        for(auto& fp:fpos){
            dist(i,j)=(fp-tp).norm();
            angle=std::min(angle,
                acos(std::max(-1.0,std::min(1.0,
                    (tp-fp).block<2,1>(0,0,2,1).normalized().dot(forward.block<2,1>(0,0,2,1))
                )))
            );
            i++;
        }
        if(angle>thetaPrioritizedAimLeaker){
            isLeaker=true;
        }
        if(isLeaker){
            leakers.push_back(std::make_pair(distFromLine,j));
        }
        j++;
    }
    std::sort(leakers.begin(),leakers.end(),
        [](const std::pair<double,int> &lhs,const std::pair<double,int> &rhs){
        return lhs.first<rhs.first;
    });
    int numAssigned=0;
    //typedef std::pair<double,int> viType;
    for(auto& li:leakers){
        nearestF.clear();
        for(int i=0;i<fpos.size();++i){
            nearestF.push_back(std::make_pair(dist(i,li.second),i));
        }
        std::sort(nearestF.begin(),nearestF.end(),
            [](const std::pair<double,int> &lhs,const std::pair<double,int> &rhs){
            return lhs.first<rhs.first;
        });
        for(auto& nf:nearestF){
            if(!assigned[nf.second]){
                numAssigned++;
                if(nf.second==0){
                    target=*candidates[li.second];
                    isAssignedAsInterceptor=true;
                }
                break;
            }
        }
        if(numAssigned==fpos.size()){
            break;
        }
    }
    //(2)最も近い敵(重複可)
    std::vector<TrackInfo*> nearestE;
    for(int i=0;i<fpos.size();++i){
        int idx;
        Eigen::VectorXd tmp=dist.block(i,0,1,tpos.size()).transpose();
        tmp.minCoeff(&idx);
        nearestE.push_back(candidates[idx]);
    }
    target=*nearestE[0];
    return;
}
void R3InitialFighterAgent01::decideShoot(TrackInfo& tgt){
    launchFlag=true;
    launchedTarget=tgt.copy();
    launchedTime=getT();
}
bool R3InitialFighterAgent01::chooseTransitionNormal(){
	/*通常時の状態遷移(前進、接近、横行)の選択を行う。状況に応じて確率で決定
	1. 見えている敵がいない場合、確定で前進
    2. 敵陣に十分近いか、敵陣側扇範囲内に敵がいないのいずれかであればライン突破を優先し前進、
    　主目標が突破阻止を目的として割り当てられているとき、突破阻止を優先し接近を選択する。
    　両方満たす場合、自身が先に突破しそうであれば突破を優先し前進を選択、相手が先に突破しそうであれば突破阻止を優先し接近を選択
	3. 主目標を味方が余裕をもって捉えていれば前進、接近、横行から確率で選択
	4. 主目標を味方が余裕をもって捉えておらず、自身で余裕をもって捉えていなければ確定で接近
	5. 主目標を味方が余裕をもって捉えておらず、自身で余裕をもって捉えていれば横行または接近から確率で選択
	Returns:
		bool: Trueのとき、もう一度回す。
	*/
    const nl::json& fObs=parent->observables;
    //1. 見えている相手がいない
    if(allEnemies.size()==0){
        if(transitionCause!=TransitionCause::NO_ENEMY){
            transitionCause=TransitionCause::NO_ENEMY;
            immidiateTransition(State::ADVANCE);
        }
        return false;
    }
    //2. ライン突破を優先 or 突破阻止を優先
    double dist=dLine-pos.dot(forward);
    double angle=M_PI;
    bool tryBreak=false;
    for(auto& t:allEnemies){
        angle=std::min(angle,acos(std::max(-1.0,std::min(1.0,(t.posI()-pos).block<2,1>(0,0,2,1).normalized().dot(forward.block<2,1>(0,0,2,1))))));
    }
    tryBreak=dist<dPrioritizedAdvance*dLine || angle>thetaPrioritizedAdvance;
    if(isAssignedAsInterceptor){
        if(tryBreak){
            double eDist=dLine-target.posI().dot(-forward);
            if(eDist<dist){
                //相手の方が先に突破する可能性が高い場合、突破阻止を優先
                if(transitionCause!=TransitionCause::INTERCEPT_LEAKER){
                    transitionCause=TransitionCause::INTERCEPT_LEAKER;
                    immidiateTransition(State::APPROACH_TARGET);
                }
                return false;
            }
        }
    }
    if(tryBreak){
        //自分の方が先に突破する可能性が高い場合、突破を優先
        if(transitionCause!=TransitionCause::TRY_BREAK){
            transitionCause=TransitionCause::TRY_BREAK;
            immidiateTransition(State::ADVANCE);
        }
        return false;
    }
    //3〜5
    if(target.nonLimitTrackers.size()>=2){
        //3. 味方が余裕をもって捉えている
        if(transitionCause!=TransitionCause::TRACKING_BY_ALLY){
            std::vector<nl::json> candidates={enumToJson(State::ADVANCE),enumToJson(State::APPROACH_TARGET),enumToJson(State::KEEP_SENSING)};
            Eigen::VectorXd probs(3);
            probs<<pAdvanceAlly,pApproachAlly,pKeepSensingAlly;
            probs/=probs.sum();
            nl::json config={
                {"type","choice"},
                {"weights",probs},
                {"candidates",candidates}
            };
            auto nextState=jsonToEnum<State>(getValueFromJsonR(config,randomGen));
            transitionCause=TransitionCause::TRACKING_BY_ALLY;
            immidiateTransition(nextState);
        }
        return false;
    }else if(target.nonLimitTrackers.size()==1){
        if(target.nonLimitTrackers[0]!=parent->getFullName()){
            //3. 味方が余裕をもって捉えている
            if(transitionCause!=TransitionCause::TRACKING_BY_ALLY){
                std::vector<nl::json> candidates={enumToJson(State::ADVANCE),enumToJson(State::APPROACH_TARGET),enumToJson(State::KEEP_SENSING)};
                Eigen::VectorXd probs(3);
                probs<<pAdvanceAlly,pApproachAlly,pKeepSensingAlly;
                probs/=probs.sum();
                nl::json config={
                    {"type","choice"},
                    {"weights",probs},
                    {"candidates",candidates}
                };
                auto nextState=jsonToEnum<State>(getValueFromJsonR(config,randomGen));
                transitionCause=TransitionCause::TRACKING_BY_ALLY;
                immidiateTransition(nextState);
            }
            return false;
        }else{
            //4. 自分だけが余裕をもって捉えている
            if(transitionCause!=TransitionCause::TRACKING_BY_MYSELF){
                std::vector<nl::json> candidates={enumToJson(State::APPROACH_TARGET),enumToJson(State::KEEP_SENSING)};
                Eigen::VectorXd probs(2);
                probs<<pApproachMyself,pKeepSensingMyself;
                probs/=probs.sum();
                nl::json config={
                    {"type","choice"},
                    {"weights",probs},
                    {"candidates",candidates}
                };
                auto nextState=jsonToEnum<State>(getValueFromJsonR(config,randomGen));
                transitionCause=TransitionCause::TRACKING_BY_MYSELF;
                immidiateTransition(nextState);
            }
            return false;
        }
    }else{
        //5. 誰も余裕をもって捉えていない
        if(transitionCause!=TransitionCause::TRACKING_BY_NOBODY){
            transitionCause=TransitionCause::TRACKING_BY_NOBODY;
            immidiateTransition(State::APPROACH_TARGET);
        }
        return false;
    }
}
void R3InitialFighterAgent01::deploy(py::object action){
    launchFlag=false;
    updateMyInfo();
    updateTracks(true);
    updateTargetStatus(true);
    selectTarget();
    bool again=deploySub();
    while(again){
        again=deploySub();
    }
    isAfterDeploy=true;
}

bool R3InitialFighterAgent01::deploySub(){
	/*
	遷移待機の終了判定⇛回避判定⇛離脱判定⇛射撃判定⇛通常遷移の順に判定
	特に遅延遷移の問題で、一度のdeployで二周以上回すこともある。
	Returns:
		bool: Trueのとき、もう一度回す。
	*/
	//遷移待ち中
	if(waitingTransition){
		if(chkTransitionCount()){//遷移待ち時間の経過
			//一旦遷移を完了させてもう一度回す。
			completeTransition();
			return true;
        }
    }
	//回避の判定(回避は最優先)
	if(chkMWS()){
		if(state!=State::EVADE){
			immidiateTransition(State::EVADE);
        }
		return false;
    }
	//離脱の判定(離脱する場合も射撃は行うためすぐにreturnはしない)
    bool needToWithdraw=false;
	bool isWithdrawing= state==State::WITHDRAW || (waitingTransition && transitionAfter==State::WITHDRAW);
	if(isWithdrawing){
		needToWithdraw=true;
        double t=manager->getTime();
		if(t>=withdrawnTime+tWithdraw){
			needToWithdraw=false;
        }
    }
	std::vector<bool> bs=chkBreak();
    std::vector<bool> breakFlag;
    bool bAny=false;
    for(auto&& b:bs){
        breakFlag.push_back(b);
        bAny=bAny||b;
    }
    if(bAny){
		if(state!=State::WITHDRAW){
			withdrawnTrigger.clear();
            for(int i=0;i<allEnemies.size();++i){
                if(breakFlag[i]){
                    withdrawnTrigger.push_back(allEnemies[i].copy());
                }
            }
			immidiateTransition(State::WITHDRAW);
        }
		withdrawnTime=manager->getTime(); //時間経過による離脱終了の起点の時刻だけは更新
		needToWithdraw=true;
    }
	//射撃の判定
	if(chkLaunchable()){//自機が射撃可能状態
        std::vector<std::pair<bool,std::pair<TrackInfo,double>>> res;
        for(auto&& t:track){
            res.push_back(chkConditionForShoot(t));
        }
        std::sort(res.begin(),res.end(),
            [](const std::pair<bool,std::pair<TrackInfo,double>>& lhs,const std::pair<bool,std::pair<TrackInfo,double>>& rhs){
            if(lhs.first){
                if(rhs.first){
                    return lhs.second.second<rhs.second.second;
                }else{
                    return true;
                }
            }else{
                if(rhs.first){
                    return false;
                }else{
                    return lhs.second.second<rhs.second.second;
                }
            }
        });
        if(res.size()>0 && res[0].first){
            decideShoot(res[0].second.first);
        }
    }
	if(needToWithdraw){
		return false;
    }
	//離脱も回避もしなかった場合、通常時の行動選択を実施
	return chooseTransitionNormal();
}

void R3InitialFighterAgent01::control(){
	/*stateに応じた制御出力を決定する。
	* stateに応じて進みたい方向dstDirと進みたい速さdstVを計算する。
    * dstDirに対しては、現時点の進行方向から導出される回転方向に回転するような角速度を発生させる。
	* dstVに対しては、現時点の速度から導出される加速度を発生させる。
	* 高度維持、マルチセンシングもこの関数内でdstDirに補正をかけることで行う。
	* 目標への接近はその瞬間に目標がいる方向へ最大速度とする。
	*/
    if(!isAfterDeploy){
		updateMyInfo();//自機情報の更新
		updateTracks(false);//航跡情報の更新
		updateTargetStatus(false);//目標情報の更新
		isAfterDeploy=false;
    }
    MotionState myMotion(parent->observables.at("motion"));
    Eigen::Vector3d ex=myMotion.relBtoP(Eigen::Vector3d(1,0,0));
    Eigen::Vector3d dstDirH;
    //水平方向の決定
    if(state==State::ADVANCE){
		//前進
		dstDirH=forward;
    }else if(state==State::APPROACH_TARGET){
		//その瞬間に目標がいる方向
		Eigen::Vector3d tpos=target.posI();
		dstDirH=(tpos-pos);
        dstDirH(2)=0;
        dstDirH.normalize();
    }else if(state==State::KEEP_SENSING){
		//横行ははじめから複数捉えることを考慮する
		//主目標方向を正面として、左右どちらに進むかを決める
		//(1)場外に出そうならば内側向き(dOutLimitKeepSensing)
		//(2)場外に出ないならば副目標を多数捉えられる向きを優先
		//(3)副目標が同数ならば前進優先
		Eigen::Vector3d tDir=(target.posI()-pos);
		double az=atan2(tDir(1),tDir(0));
		Eigen::Vector3d right=Eigen::Vector3d(cos(az+M_PI/2.-thetaKeepSensing),sin(az+M_PI/2.-thetaKeepSensing),0.);
		Eigen::Vector3d left=Eigen::Vector3d(cos(az-M_PI/2.+thetaKeepSensing),sin(az-M_PI/2.+thetaKeepSensing),0.);
		if(abs(pos(0))>=dOut-dOutLimitKeepSensing){
			//(1)
            Eigen::Vector3d inner;
			if(pos(0)>0){
				inner<<-1.,0.,0.;
            }else{
				inner<<1.,0.,0.;
            }
			double r=right.dot(inner);
			double l=left.dot(inner);
			if(r>=l){
				dstDirH=right;
            }else{
				dstDirH=left;
            }
        }else{
			//(2)
			int leftCount=0;
			int rightCount=0;
            for(auto& t:additionalTargets){
				tDir=(t.posI()-pos).array()*Eigen::Array3d(1,1,0);
                double sAngle=parent->observables.at("/spec/sensor/radar/thetaFOR"_json_pointer);
				if(tDir.dot(right)>=cos(sAngle)){
					rightCount++;
                }
				if(tDir.dot(left)>=cos(sAngle)){
					leftCount++;
                }
            }
			if(rightCount>leftCount){
				dstDirH=right;
            }else if(leftCount>rightCount){
				dstDirH=left;
            }else{
				//(3)
				double r=right.dot(forward);
				double l=left.dot(forward);
				if(r>=l){
				    dstDirH=right;
                }else{
					dstDirH=left;
                }
            }
        }
    }else if(state==State::WITHDRAW){
		//基本的には離脱のきっかけとなった敵から最も離れる方位に向かう
        std::vector<Eigen::Vector3d> tpos;
        for(auto& t:allEnemies){
            for(auto& trig:withdrawnTrigger){
                if(t.isSame(trig)){
                    tpos.push_back(t.posI()-pos);
                    break;
                }
            }
        }
        if(tpos.size()==0){
			//対象の敵が見えなくなったら真後ろへ
			dstDirH=-forward;
        }else{
            std::vector<double> az;
            for(auto& p:tpos){
                az.push_back(atan2(p(1),p(0)));
            }
            std::sort(az.begin(),az.end());
            az.push_back(az[0]+2*M_PI);
            Eigen::VectorXd delta=Eigen::VectorXd::Zero(az.size()-1);
            for(int i=0;i<az.size()-1;++i){
                delta(i)=az[i+1]-az[i];
            }
            int idx=0;
            delta.minCoeff(&idx);
			double dstAz=(az[idx]+az[idx+1])/2.0;
			dstDirH=Eigen::Vector3d(cos(dstAz),sin(dstAz),0.0);
        }
    }else if(state==State::EVADE){
		//自身の正面に一番近いものに背を向ける。見えなくなっていたら現状維持
		if(chkMWS()){
            std::vector<std::pair<double,Eigen::Vector3d>> tmp;
            for(auto& m:parent->observables.at("/sensor/mws/track"_json_pointer)){
                Eigen::Vector3d dir=m.get<Track2D>().dirI();
                tmp.push_back(std::make_pair(-dir.dot(myMotion.relBtoP(Eigen::Vector3d(1,0,0))),-dir));
            }
            Eigen::Vector3d dr=(*std::min_element(tmp.begin(),tmp.end(),
                [](const std::pair<double,Eigen::Vector3d> &lhs,const std::pair<double,Eigen::Vector3d> &rhs){
                return lhs.first<rhs.first;
            })).second;
            dstDirH=dr;
            dstDirH(2)=0;
            dstDirH.normalize();
        }else{
			dstDirH=vel;
            dstDirH(2)=0;
            dstDirH.normalize();
        }
    }else{
		std::cout<<"Invalid state. state="<<magic_enum::enum_name(state)<<std::endl;
    }
	//副目標追尾のための方位補正(前進、接近)
	if(state==State::ADVANCE || state==State::APPROACH_TARGET){
		//前進or接近のときは、一定角度以内の補正で副目標が捉えられるならば、補正する。
		int leftCount=0;
		double leftDelta=0.0;
		int rightCount=0;
		double rightDelta=0.0;
		double az=atan2(dstDirH(1),dstDirH(0));
        for(auto& t:additionalTargets){
			Eigen::Vector3d tDir=(t.posI()-pos).array()*Eigen::Array3d(1,1,0);
			double delta=asin(std::max(-1.0,std::min(1.0,Eigen::Vector3d(dstDirH.array()*Eigen::Array3d(1,1,0)).cross(tDir)(2))));
            double sAngle=parent->observables.at("/spec/sensor/radar/thetaFOR"_json_pointer);
			double needAngle=(delta>=0) ? std::max(0.0,delta-sAngle) : std::max(0.0,delta+sAngle);
			if(abs(needAngle)<=thetaModForSensing){
				if(needAngle>=0){
						rightDelta=std::max(rightDelta,needAngle);
						rightCount++;
                }else{
						leftDelta=std::max(leftDelta,-needAngle);
						leftCount++;
                }
            }
        }
		if(leftCount>0 || rightCount>0){
			if(leftCount>rightCount){
				az-=leftDelta;
            }else if(leftCount<rightCount){
				az+=rightDelta;
            }else if(leftDelta>=rightDelta){
				az-=leftDelta;
            }else{
				az+=rightDelta;
            }
        }
		dstDirH<<cos(az),sin(az),0;
    }
	if(abs(pos(0))>=dOut-dOutLimit){
		//場外防止のための方位補正
		//判定ラインの超過具合に応じて復帰角度を変化させる。(無限遠でラインに直交、ライン上でラインと平行)
		double over=(abs(pos(0))-dOut)/dOutLimit;
		double theta=atan(over);
		double cs=cos(theta);
		double sn=sin(theta);
		if(pos(0)>0){//北側
			if(dstDirH(1)>0){//東向き
				if(atan2(-dstDirH(0),dstDirH(1))<theta){
					dstDirH<<-sn,cs,0;
                }
        	}else{//西向き
				if(atan2(-dstDirH(0),-dstDirH(1))<theta){
					dstDirH<<-sn,-cs,0;
                }
            }
		}else{//南側
			if(dstDirH(1)>0){//東向き
				if(atan2(dstDirH(0),dstDirH(1))<theta){
					dstDirH<<sn,cs,0;
                }
            }else{//西向き
				if(atan2(dstDirH(0),-dstDirH(1))<theta){
					dstDirH<<sn,-cs,0;
                }
            }
        }
    }
    //鉛直方向の決定
    double pitch=0.0;
    if(state==State::EVADE){
        //下限高度でピッチ0、下限高度の2倍以上で指定ピッチ角となるようにする。
        //下限高度を下回る場合も裏返しで下限高度に近づくようにする。
        double alt=-pos(2);
        pitch=thetaEvasion*(std::max(0.0,std::min(hEvasion*2,alt))-hEvasion)/hEvasion;
	}else{
		//回避時以外の高度維持
        double hN=hNormal;
		pitch=(hN+pos(2))/10.0;
		pitch=std::max(-thetaStable,std::min(thetaStable,deg2rad(pitch)));
    }
	double cs=cos(pitch);
    double sn=sin(pitch);
	dstDir=Eigen::Vector3d(cs*dstDirH(0),cs*dstDirH(1),-sn).normalized();
    if(launchedTarget.is_none() || launchedTarget.state==TrackInfo::UpdateState::LOST){
		fireTgt=TrackInfo();
    }else{
		fireTgt=TrackInfo(launchedTarget);
    }
    makeCommand();
}
void R3InitialFighterAgent01::makeCommand(){
    double V=vel.norm();
    if(parent->isinstance<CoordinatedFighter>()){
        MotionState myMotion(parent->observables.at("motion"));
        Eigen::Vector3d ex=myMotion.relBtoP(Eigen::Vector3d(1,0,0));
        Eigen::Vector3d exh(ex(0),ex(1),0);
        exh.normalize();
        Eigen::Vector3d dstDirH=dstDir;
        dstDirH(2)=0;
        dstDirH.normalize();
        double pitch=asin(dstDir(2));
        Eigen::Vector3d cmdDstDir=dstDir;
        double cs=exh.dot(dstDirH);
        Eigen::Vector3d turnAxis=exh.cross(dstDirH);
        //回転中に場外に出そうな場合は回転軸を反転
        Eigen::Vector3d outside;
        if(abs(pos(0))>=dOut-dOutLimitTurnAxis){
            if(pos(0)>0){
                //北側
                outside<<1,0,0;
            }else{
                //南側
                outside<<-1,0,0;
            }
            if(turnAxis.dot(exh.cross(outside))>=0 && turnAxis.dot(outside.cross(dstDirH))>=0){
                turnAxis=-turnAxis;
            }
        }
        double sn=turnAxis.norm();
        double theta=atan2(sn,cs);
        Eigen::Vector3d side=turnAxis.cross(exh).normalized();
        if(theta>M_PI/4){
            theta=M_PI/4;
            cmdDstDir=(exh*cos(theta)+side*sin(theta)).normalized();
            cmdDstDir<<cmdDstDir(0)*cos(pitch),cmdDstDir(1)*cos(pitch),sin(pitch);
        }
        commands[parent->getFullName()]={
            {"motion",{
                {"dstDir",cmdDstDir},
                {"dstAccel",nominalAccelCmd}
            }},
            {"weapon",{
                {"launch",launchFlag},
                {"target",fireTgt}
            }}
        };
        observables[parent->getFullName()]["decision"]={
            {"Roll",nl::json::array({"Don't care"})},
            {"Horizontal",nl::json::array({"Az_NED",atan2(dstDir(1),dstDir(0))})},
            {"Vertical",(state==State::EVADE)?nl::json::array({"El",-thetaEvasion}):nl::json::array({"Pos",-hNormal})},
            {"Throttle",nl::json::array({"Accel",nominalAccelCmd})},
            {"Fire",nl::json::array({launchFlag,fireTgt})}
        };
        if(state==State::EVADE){
            //回避時はmaxAB
            commands[parent->getFullName()]["motion"].merge_patch(
                {
                    {"dstAccel",nullptr},
                    {"dstThrottle",1.0}
                }
            );
            observables[parent->getFullName()]["decision"]["Throttle"]=nl::json::array({"Throttle",1.0});
        }else{
            if(V<minimumV){
                velRecovery=true;
            }
            if(V>=minimumRecoveryV){
                velRecovery=false;
            }
            if(velRecovery){
                commands[parent->getFullName()]["motion"].merge_patch(
                    {
                        {"dstAccel",nullptr},
                        {"dstV",minimumRecoveryDstV}
                    }
                );
                observables[parent->getFullName()]["decision"]["Throttle"]=nl::json::array({"Vel",minimumRecoveryDstV});
            }

        }
    }else if(parent->isinstance<MassPointFighter>()){
	    double dstV=parent->observables.at("/spec/dynamics/vMax"_json_pointer);//基本的には最大速度とする
        Eigen::Vector3d Vn=vel/V;
        double cs=Vn.dot(dstDir);
        Eigen::Vector3d delta=Vn.cross(dstDir);
        double sn=delta.norm();
        double theta=atan2(sn,cs);
        if(theta<1e-6){
            omegaI<<0,0,0;
        }else if(theta>M_PI-1e-6){
            Eigen::Vector3d pz=(Vn.cross(Eigen::Vector3d(0,0,1).cross(Vn))).normalized();
            omegaI=pz*M_PI;
        }else{
            omegaI=delta*(theta/sn);
        }
        double accel=dstV-V;
        omegaB=MotionState(parent->observables.at("motion")).relPtoB(omegaI);
        commands[parent->getFullName()]={
            {"motion",{
                {"roll",omegaB(0)*omegaScale(0)},
                {"pitch",omegaB(1)*omegaScale(1)},
                {"yaw",omegaB(2)*omegaScale(2)},
                {"throttle",accel}
            }},
            {"weapon",{
                {"launch",launchFlag},
                {"target",fireTgt}
            }}
        };
        observables[parent->getFullName()]["decision"]={
            {"Roll",nl::json::array({"Don't care"})},
            {"Horizontal",nl::json::array({"Az_NED",atan2(dstDir(1),dstDir(0))})},
            {"Vertical",(state==State::EVADE)?nl::json::array({"El",-thetaEvasion}):nl::json::array({"Pos",-hNormal})},
            {"Throttle",nl::json::array({"Accel",accel})},
            {"Fire",nl::json::array({launchFlag,fireTgt})}
        };
    }else{
        throw std::runtime_error("R3InitialFighterAgent01 accepts only MassPointFighrer or CoordinatedFighter.");
    }
}
double R3InitialFighterAgent01::getT(){
    return manager->getTime();
}
double R3InitialFighterAgent01::calcRHead(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    std::shared_ptr<FighterAccessor> f=std::dynamic_pointer_cast<FighterAccessor>(parent);
    return f->getRmax(rs,vs,rt,vt,0.0);
}
double R3InitialFighterAgent01::calcRTail(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    std::shared_ptr<FighterAccessor> f=std::dynamic_pointer_cast<FighterAccessor>(parent);
    return f->getRmax(rs,vs,rt,vt,M_PI);
}

void exportR3InitialFighterAgent01(py::module& m)
{
    using namespace pybind11::literals;
    {
    auto cls=py::class_<TrackInfo,Track3D,TrackInfoWrap<>,std::shared_ptr<TrackInfo>>(m,"TrackInfo");
    cls
    .def(py::init<>())
    .def(py::init<const Track3D&,int>())
    .def(py::init<const nl::json&>())
    .def(py::init([](const py::object& obj){return TrackInfo(obj);}))
    DEF_FUNC(TrackInfo,copy)
    DEF_FUNC(TrackInfo,update)
    DEF_FUNC(TrackInfo,to_json)
    DEF_READWRITE(TrackInfo,idx)
    DEF_READWRITE(TrackInfo,distance)
    DEF_READWRITE(TrackInfo,myRHead)
    DEF_READWRITE(TrackInfo,myRTail)
    DEF_READWRITE(TrackInfo,hisRHead)
    DEF_READWRITE(TrackInfo,hisRTail)
    DEF_READWRITE(TrackInfo,inOurSensor)
    DEF_READWRITE(TrackInfo,inMySensor)
    DEF_READWRITE(TrackInfo,numTracker)
    DEF_READWRITE(TrackInfo,numTrackerLimit)
    DEF_READWRITE(TrackInfo,trackers)
    DEF_READWRITE(TrackInfo,limitTrackers)
    DEF_READWRITE(TrackInfo,nonLimitTrackers)
    DEF_READWRITE(TrackInfo,state)
    DEF_READWRITE(TrackInfo,memoryStartTime)
    ;
    py::enum_<TrackInfo::SensingState>(cls,"SensingState")
    .value("INSIDE",TrackInfo::SensingState::INSIDE)
    .value("LIMIT",TrackInfo::SensingState::LIMIT)
    .value("OUTSIDE",TrackInfo::SensingState::OUTSIDE)
    ;
    py::enum_<TrackInfo::UpdateState>(cls,"UpdateState")
    .value("TRACK",TrackInfo::UpdateState::TRACK)
    .value("MEMORY",TrackInfo::UpdateState::MEMORY)
    .value("LOST",TrackInfo::UpdateState::LOST)
    ;
    }
    {
    auto cls=EXPOSE_CLASS(R3InitialFighterAgent01);
    cls
    DEF_FUNC(R3InitialFighterAgent01,validate)
    DEF_FUNC(R3InitialFighterAgent01,deploy)
    DEF_FUNC(R3InitialFighterAgent01,control)
    DEF_READWRITE(R3InitialFighterAgent01,state)
    ;
    py::enum_<R3InitialFighterAgent01::State>(cls,"State")
    .value("ADVANCE",R3InitialFighterAgent01::State::ADVANCE)
    .value("APPROACH_TARGET",R3InitialFighterAgent01::State::APPROACH_TARGET)
    .value("KEEP_SENSING",R3InitialFighterAgent01::State::KEEP_SENSING)
    .value("WITHDRAW",R3InitialFighterAgent01::State::WITHDRAW)
    .value("EVADE",R3InitialFighterAgent01::State::EVADE)
    .value("NONE",R3InitialFighterAgent01::State::NONE)
    ;
    }
}
