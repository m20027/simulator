#include "Fighter.h"
#include "Utility.h"
#include "Units.h"
#include "SimulationManager.h"
#include "Agent.h"
#include "Missile.h"
#include "Sensor.h"
#include "CommunicationBuffer.h"
#include <boost/uuid/nil_generator.hpp>
using namespace util;
Fighter::Fighter(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:PhysicalAsset(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    //modelConfigで指定するもの
    if(modelConfig.contains("stealth")){
        rcsScale=getValueFromJsonKRD(modelConfig.at("stealth"),"rcsScale",randomGen,1.0);
    }else{
        rcsScale=1.0;
    }
    if(modelConfig.contains("weapon")){
        numMsls=getValueFromJsonKRD(modelConfig.at("weapon"),"numMsls",randomGen,10);
    }else{
        numMsls=10;
    }
    //instanceConfigで指定するもの
    motion.pos=getValueFromJsonKRD(instanceConfig,"pos",randomGen,Eigen::Vector3d(0,0,0));
    double V=getValueFromJsonKRD(instanceConfig,"vel",randomGen,0.0);
    motion.az=deg2rad(getValueFromJsonKRD(instanceConfig,"heading",randomGen,0.0));
    datalinkName=getValueFromJsonKRD<std::string>(instanceConfig,"datalinkName",randomGen,"");
    //その他の位置、姿勢等の運動状態に関する変数の初期化
    motion.el=0.0;
    vel_prev=motion.vel=Eigen::Vector3d(V*cos(motion.az),V*sin(motion.az),0);
    pos_prev=motion.pos-motion.vel*manager->getBaseTimeStep();
    motion.q=Quaternion::fromBasis(
        Eigen::Vector3d(cos(motion.az),sin(motion.az),0),
        Eigen::Vector3d(-sin(motion.az),cos(motion.az),0),
        Eigen::Vector3d(0,0,1));
    motion.calcQh();
    motion.omega<<0,0,0;
    //その他の内部変数
    isDatalinkEnabled=true;
    track.clear();
    trackSource.clear();
    target=Track3D();targetID=-1;
    launchable=true;
    observables["motion"]=motion;
    observables["spec"]={
        {"dynamics",{nl::json::object()}},
        {"weapon",{
            {"numMsls",numMsls}
        }},
        {"stealth",{
            {"rcsScale",rcsScale}
        }},
        {"sensor",nl::json::object()},
        {"propulsion",nl::json::object()}
    };
    observables["shared"]={
        {"agent",nl::json::object()},
        {"fighter",nl::json::object()}
    };
}
Fighter::~Fighter(){
}
void Fighter::makeChildren(){
    nl::json sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":Radar"},//PhysicalAsset
        {"parent",this->weak_from_this()},//PhysicalAsset
        {"isBound",true}//PhysicalAsset
    };
    radar=manager->generateAsset<AircraftRadar>(
        "PhysicalAsset",
        modelConfig.at("/sensor/radar"_json_pointer),
        sub
    );
    observables["spec"]["sensor"]["radar"]=radar.lock()->observables["spec"];
    sub["seed"]=randomGen();
    sub["fullName"]=fullName+":MWS";
    mws=manager->generateAsset<MWS>(
        "PhysicalAsset",
        modelConfig.at("/sensor/mws"_json_pointer),
        sub
    );
    observables["spec"]["sensor"]["mws"]=mws.lock()->observables["spec"];
    missiles.clear();
    missileTargets.clear();
    nextMsl=0;
    remMsls=numMsls;
    sub["isBound"]=false;
    for(int i=0;i<numMsls;++i){
        sub["seed"]=randomGen();
        sub["fullName"]=fullName+":Missile"+std::to_string(i+1);
        missiles.push_back(
            manager->generateAsset<Missile>(
                "PhysicalAsset",
                modelConfig.at("/weapon/missile"_json_pointer),
                sub
            )
        );
        missileTargets.push_back(std::make_pair(Track3D(),false));
        manager->generateCommunicationBuffer(
            "MissileComm:"+sub["fullName"].get<std::string>(),
            nl::json::array({"PhysicalAsset:"+getFullName(),"PhysicalAsset:"+sub["fullName"].get<std::string>()}),
            nl::json::array()
        );
    }
    if(numMsls<=0){
        //初期弾数0のとき、射程計算のみ可能とするためにダミーインスタンスを生成
        sub["seed"]=randomGen();
        sub["fullName"]=fullName+":Missile(dummy)";
        dummyMissile=manager->generateAsset<Missile>(
                "PhysicalAsset",
                modelConfig.at("/weapon/missile"_json_pointer),
                sub
            );
        //CommunicationBufferは正規表現で対象Assetを特定するため、括弧はエスケープが必要
        std::string query=fullName+":Missile\\(dummy\\)";
        manager->generateCommunicationBuffer(
            "MissileComm:"+sub["fullName"].get<std::string>(),
            nl::json::array({"PhysicalAsset:"+getFullName(),"PhysicalAsset:"+query}),
            nl::json::array()
        );
    }else{
        dummyMissile=std::shared_ptr<Missile>(nullptr);
    }
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":SensorDataSharer"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["SensorDataSharer"]=manager->generateAssetByClassName<SensorDataSharer>("Controller","Fighter::SensorDataSharer",{},sub);
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":SensorDataSanitizer"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["SensorDataSanitizer"]=manager->generateAssetByClassName<SensorDataSanitizer>("Controller","Fighter::SensorDataSanitizer",{},sub);
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":OtherDataSharer"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["OtherDataSharer"]=manager->generateAssetByClassName<OtherDataSharer>("Controller","Fighter::OtherDataSharer",{},sub);
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":OtherDataSanitizer"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["OtherDataSanitizer"]=manager->generateAssetByClassName<OtherDataSanitizer>("Controller","Fighter::OtherDataSanitizer",{},sub);
    if(modelConfig.contains("/pilot/model"_json_pointer)){
        sub={
            {"seed",randomGen()},//Entity
            {"fullName",fullName+":HumanIntervention"},//Controller
            {"parent",this->weak_from_this()}//Controller
        };
        controllers["HumanIntervention"]=manager->generateAsset<Controller>("Controller",modelConfig.at("/pilot/model"_json_pointer),sub);
    }
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":WeaponController"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["WeaponController"]=manager->generateAssetByClassName<WeaponController>("Controller","Fighter::WeaponController",{},sub);
    sub={
        {"seed",randomGen()},//Entity
        {"fullName",fullName+":FlightController"},//Controller
        {"parent",this->weak_from_this()}//Controller
    };
    controllers["FlightController"]=manager->generateAsset<Controller>("Controller",modelConfig.at("/dynamics/controller"_json_pointer),sub);
}
void Fighter::validate(){
    isDatalinkEnabled = communicationBuffers.count(datalinkName)>0;
}
void Fighter::setDependency(){
    //validate
    for(auto&& e:missiles){
        e.lock()->dependencyChecker->addDependency(SimPhase::VALIDATE,getShared<Asset>(this->shared_from_this()));
    }
    if(!dummyMissile.expired()){
        dummyMissile.lock()->dependencyChecker->addDependency(SimPhase::VALIDATE,getShared<Asset>(this->shared_from_this()));
    }
    //perceive
    controllers["SensorDataSharer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,radar.lock());
    controllers["SensorDataSharer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,mws.lock());
    for(auto&& asset:manager->getAssets()){
        if(asset.lock()->getTeam()==team && isinstance<Fighter>(asset)){
            auto f=getShared<const Fighter>(asset);
            controllers["SensorDataSanitizer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,f->controllers.at("SensorDataSharer").lock());
        }
    }
    for(auto&& e:missiles){
        e.lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,controllers["SensorDataSanitizer"].lock());
        dependencyChecker->addDependency(SimPhase::PERCEIVE,e.lock());
    }
    if(!dummyMissile.expired()){
        dummyMissile.lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,controllers["SensorDataSanitizer"].lock());
        dependencyChecker->addDependency(SimPhase::PERCEIVE,dummyMissile.lock());
    }
    controllers["OtherDataSharer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,getShared<Asset>(this->shared_from_this()));
    controllers["OtherDataSharer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,agent.lock());
    for(auto&& asset:manager->getAssets()){
        if(asset.lock()->getTeam()==team && isinstance<Fighter>(asset)){
            auto f=getShared<const Fighter>(asset);
            controllers["OtherDataSanitizer"].lock()->dependencyChecker->addDependency(SimPhase::PERCEIVE,f->controllers.at("OtherDataSharer").lock());
        }
    }
    //control
    controllers["OtherDataSharer"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,agent.lock());
    for(auto&& asset:manager->getAssets()){
        if(asset.lock()->getTeam()==team && isinstance<Fighter>(asset)){
            auto f=getShared<const Fighter>(asset);
            controllers["OtherDataSanitizer"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,f->controllers.at("OtherDataSharer").lock());
        }
    }
    if(controllers.count("HumanIntervention")>0){
        controllers["HumanIntervention"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["OtherDataSanitizer"].lock());
        controllers["WeaponController"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["HumanIntervention"].lock());
        controllers["FlightController"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["HumanIntervention"].lock());
    }else{
        controllers["WeaponController"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["OtherDataSanitizer"].lock());
        controllers["FlightController"].lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["OtherDataSanitizer"].lock());
    }
    for(auto&& e:missiles){
        e.lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["WeaponController"].lock());
    }
    if(!dummyMissile.expired()){
        dummyMissile.lock()->dependencyChecker->addDependency(SimPhase::CONTROL,controllers["WeaponController"].lock());
    }
}
void Fighter::perceive(bool inReset){
    PhysicalAsset::perceive(inReset);
    nl::json mslObs=nl::json::array();
    for(auto&& e:missiles){
        mslObs.push_back(e.lock()->observables);
    }
    observables["weapon"]={
        {"remMsls",remMsls},
        {"nextMsl",nextMsl},
        {"launchable",launchable},
        {"missiles",std::move(mslObs)}
    };
}
void Fighter::control(){
    //done by controllers
}
void Fighter::behave(){
    calcMotion(manager->getBaseTimeStep());
    if(motion.pos(2)>0){//墜落
        manager->triggerEvent("Crash",this->weak_from_this());
        kill();
    }
}
void Fighter::kill(){
    radar.lock()->kill();
    mws.lock()->kill();
    if(isDatalinkEnabled){
        //immidiate notification to friends. If you need more realistic behavior, other notification or estimation of aliveness scheme should be implemented.
        communicationBuffers[datalinkName].lock()->send({
            {"fighterObservables",{{getFullName(),{
                {"isAlive",false}
            }}}}
        },CommunicationBuffer::MERGE);
    }
    for(auto&& e:controllers){
        e.second.lock()->kill();
    }
    this->PhysicalAsset::kill();
}
std::pair<bool,Track3D> Fighter::isTracking(std::weak_ptr<PhysicalAsset> target_){
    if(target_.expired()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.lock()->uuid);
    }
}
std::pair<bool,Track3D> Fighter::isTracking(const Track3D& target_){
    if(target_.is_none()){
        return std::make_pair(false,Track3D());
    }else{
        return isTracking(target_.truth);
    }
}
std::pair<bool,Track3D> Fighter::isTracking(const boost::uuids::uuid& target_){
    if(target_==boost::uuids::nil_uuid()){
        return std::make_pair(false,Track3D());
    }else{
        if(isAlive()){
            for(auto& t:track){
                if(t.isSame(target_)){
                    return std::make_pair(true,t);
                }
            }
        }
        return std::make_pair(false,Track3D());
    }
}
void Fighter::setFlightControllerMode(const std::string& ctrlName){
    std::dynamic_pointer_cast<FlightController>(controllers["FlightController"].lock())->setMode(ctrlName);
}
Eigen::Vector3d Fighter::toEulerAngle(){
    Eigen::Vector3d ex=relItoB(Eigen::Vector3d(1,0,0));
    Eigen::Vector3d ey=relItoB(Eigen::Vector3d(0,1,0));
    Eigen::Vector3d horizontalY=Eigen::Vector3d(0,0,1).cross(ex).normalized();
    double sinRoll=horizontalY.cross(ey).dot(ex);
    double cosRoll=horizontalY.dot(ey);
    double rollAtt=atan2(sinRoll,cosRoll);
    return Eigen::Vector3d(rollAtt,-motion.el,motion.az);
}
double Fighter::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    if(numMsls>0){
        return missiles[0].lock()->getRmax(rs,vs,rt,vt);
    }else{
        return dummyMissile.lock()->getRmax(rs,vs,rt,vt);
    }
}
double Fighter::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
    if(numMsls>0){
        return missiles[0].lock()->getRmax(rs,vs,rt,vt,aa);
    }else{
        return dummyMissile.lock()->getRmax(rs,vs,rt,vt,aa);
    }
}
Eigen::Vector3d Fighter::relHtoI(const Eigen::Vector3d &v) const{
    return motion.relHtoP(v);
}
Eigen::Vector3d Fighter::relItoH(const Eigen::Vector3d &v) const{
    return motion.relPtoH(v);
}
Eigen::Vector3d Fighter::absHtoI(const Eigen::Vector3d &v) const{
    return motion.absHtoP(v);
}
Eigen::Vector3d Fighter::absItoH(const Eigen::Vector3d &v) const{
    return motion.absPtoH(v);
}
std::shared_ptr<AssetAccessor> Fighter::getAccessor(){
    if(!accessor){
        accessor=std::make_shared<FighterAccessor>(getShared<Fighter>(this->shared_from_this()));
    }
    return accessor;
}

Fighter::SensorDataSharer::SensorDataSharer(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
void Fighter::SensorDataSharer::perceive(bool inReset){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        if(p->isDatalinkEnabled){
            p->communicationBuffers[p->datalinkName].lock()->send({
                {"fighterObservables",{{p->getFullName(),{
                    {"sensor",{
                        {"radar",p->radar.lock()->observables},
                        {"mws",p->mws.lock()->observables}
                    }},
                    {"time",manager->getTime()}
                }}}}
            },CommunicationBuffer::MERGE);
        }
    }
}
Fighter::SensorDataSanitizer::SensorDataSanitizer(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
void Fighter::SensorDataSanitizer::perceive(bool inReset){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        p->track.clear();
        p->trackSource.clear();
        for(auto& e:p->radar.lock()->track){
            p->track.push_back(e.copy());
            p->trackSource.push_back({p->getFullName()});
        }
        nl::json sharedFighterObservable=nl::json::object();
        p->observables["shared"]["fighter"]=nl::json::object();
        if(p->isDatalinkEnabled){
            std::pair<double,nl::json> tmp=p->communicationBuffers[p->datalinkName].lock()->receive("fighterObservables");
            if(tmp.first>=0){//valid data
                sharedFighterObservable=tmp.second;
            }
            Track3D same;
            int sameID=-1;
            int idx=0;
            for(auto&& e:sharedFighterObservable.items()){
                if(e.key()!=p->getFullName()){
                    double sent=e.value().at("time");
                    if(lastSharedTime.count(e.key())==0 || lastSharedTime[e.key()]<sent){
                        lastSharedTime[e.key()]=sent;
                        std::vector<Track3D> shared=e.value().at("/sensor/radar/track"_json_pointer);
                        p->observables["shared"]["fighter"][e.key()]["sensor"]=e.value()["sensor"];
                        for(auto&& rhs:shared){
                            same=Track3D();
                            sameID=-1;
                            idx=0;
                            for(auto& lhs:p->track){
                                if(lhs.isSame(rhs)){
                                    same=lhs;
                                    sameID=idx;
                                    idx++;
                                }
                            }
                            if(same.is_none()){
                                p->track.push_back(rhs);
                                p->trackSource.push_back({e.key()});
                            }else{
                                same.addBuffer(rhs);
                                p->trackSource[sameID].push_back(e.key());
                            }
                        }
                    }
                }
            }
            for(auto& t:p->track){
                t.merge();
            }
        }
        p->observables["sensor"]={
            {"radar",p->radar.lock()->observables},
            {"mws",p->mws.lock()->observables}
        };
        nl::json j_track=nl::json::array();
        for(auto&& e:p->track){
            j_track.push_back(Track3D(e));
        }
        p->observables["sensor"]["track"]=j_track;
        p->observables["sensor"]["trackSource"]=p->trackSource;
        if(!p->target.is_none()){
            Track3D old=p->target;
            p->target=Track3D();
            p->targetID=-1;
            int i=0;
            for(auto& t:p->track){
                if(old.isSame(t)){
                    p->target=t;
                    p->targetID=i;
                }
                ++i;
            }
        }else{
            p->targetID=-1;
        }
        for(int mslId=0;mslId<p->nextMsl;++mslId){
            Track3D old=p->missileTargets[mslId].first;
            p->missileTargets[mslId].second=false;
            for(auto& t:p->track){
                if(old.isSame(t)){
                    p->missileTargets[mslId].first=t;
                    p->missileTargets[mslId].second=true;
                    break;
                }
            }
        }
    }else{
        p->track.clear();
    }
}
Fighter::OtherDataSharer::OtherDataSharer(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
void Fighter::OtherDataSharer::perceive(bool inReset){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        if(p->isDatalinkEnabled){
            if(p->hasAgent){
                nl::json agObs=nl::json::object();
                for(auto&&e:p->agent.lock()->observables.items()){
                    agObs[e.key()]={
                        {"obs",e.value()},
                        {"time",manager->getTime()}
                    };
                }
                p->communicationBuffers[p->datalinkName].lock()->send({
                    {"agentObservables",agObs}
                },CommunicationBuffer::MERGE);
            }
            p->communicationBuffers[p->datalinkName].lock()->send({
                {"fighterObservables",{{p->getFullName(),{
                    {"isAlive",p->isAlive()},
                    {"spec",p->observables["spec"]},
                    {"motion",p->observables["motion"]},
                    {"weapon",p->observables["weapon"]},
                    {"time",manager->getTime()}
                }}}}
            },CommunicationBuffer::MERGE);
        }
    }
}
void Fighter::OtherDataSharer::control(){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_ && p->isDatalinkEnabled){
        if(p->hasAgent){
            nl::json agCom=nl::json::object();
            for(auto&&e:p->agent.lock()->commands.items()){
                agCom[e.key()]={
                    {"com",e.value()},
                    {"time",manager->getTime()}
                };
            }
            p->communicationBuffers[p->datalinkName].lock()->send({
                {"agentCommands",agCom}
            },CommunicationBuffer::MERGE);
        }
    }
}
Fighter::OtherDataSanitizer::OtherDataSanitizer(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    //lastSharedTimeOfAgentObservable=-1;
    //lastSharedTimeOfFighterObservable=-1;
    lastSharedTimeOfAgentCommand=-1;
}
void Fighter::OtherDataSanitizer::perceive(bool inReset){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        nl::json sharedAgentObservable=nl::json::object();
        nl::json sharedFighterObservable=nl::json::object();
        if(p->isDatalinkEnabled){
            std::pair<double,nl::json> tmp=p->communicationBuffers[p->datalinkName].lock()->receive("agentObservables");
            if(tmp.first>=0){//valid data
                sharedAgentObservable=tmp.second;
            }
            tmp=p->communicationBuffers[p->datalinkName].lock()->receive("fighterObservables");
            if(tmp.first>=0){//valid data
                sharedFighterObservable=tmp.second;
            }
        }
        p->observables["shared"]["agent"]=nl::json::object();
        for(auto&& e:sharedAgentObservable.items()){
            double sent=e.value().at("time");
            if(lastSharedTimeOfAgentObservable.count(e.key())==0 || lastSharedTimeOfAgentObservable[e.key()]<sent){
                lastSharedTimeOfAgentObservable[e.key()]=sent;
                p->observables["shared"]["agent"][e.key()]=sharedAgentObservable[e.key()]["obs"];
            }
        }
        if(p->hasAgent){
            p->observables["shared"]["agent"].merge_patch(p->agent.lock()->observables);
        }
        for(auto&& e:sharedFighterObservable.items()){
            double sent=e.value().at("time");
            if(lastSharedTimeOfFighterObservable.count(e.key())==0 || lastSharedTimeOfFighterObservable[e.key()]<sent){
                lastSharedTimeOfFighterObservable[e.key()]=sent;
                p->observables["shared"]["fighter"][e.key()]["isAlive"]=e.value().at("isAlive");
                p->observables["shared"]["fighter"][e.key()]["spec"]=e.value().at("spec");
                p->observables["shared"]["fighter"][e.key()]["motion"]=e.value().at("motion");
                p->observables["shared"]["fighter"][e.key()]["weapon"]=e.value().at("weapon");
            }
        }
    }
}
void Fighter::OtherDataSanitizer::control(){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        nl::json agentCommand={
            {"motion",{
                {"roll",0.0},
                {"pitch",0.0},
                {"yaw",0.0},
                {"accel",0.0}
            }},
            {"weapon",{
                {"launch",false},
                {"target",Track3D()}
            }}
        };
        if(p->hasAgent){
            agentCommand=p->agent.lock()->commands.at(p->getFullName());
        }else{
            if(p->isDatalinkEnabled){
                auto tmp=p->communicationBuffers[p->datalinkName].lock()->receive("agentCommands");
                if(tmp.first>=0){//valid data
                    auto sharedAgentCommand=tmp.second;
                    if(sharedAgentCommand.contains(p->getFullName())){
                        double sent=sharedAgentCommand.at(p->getFullName()).at("time");
                        if(lastSharedTimeOfAgentCommand<0 || lastSharedTimeOfAgentCommand<sent){
                            lastSharedTimeOfAgentCommand=sent;
                            agentCommand=sharedAgentCommand.at(p->getFullName()).at("com");
                        }
                    }
                }
            }
        }
        p->commands["fromAgent"]=agentCommand;
        p->commands["motion"]=agentCommand["motion"];//もしHumanInterventionが無い場合はそのまま使われる
        p->commands["weapon"]=agentCommand["weapon"];//もしHumanInterventionが無い場合はそのまま使われる
    }
}
Fighter::HumanIntervention::HumanIntervention(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    capacity=getValueFromJsonKRD(modelConfig,"capacity",randomGen,1);
    delay=getValueFromJsonKRD(modelConfig,"delay",randomGen,3.0);
    cooldown=getValueFromJsonKRD(modelConfig,"cooldown",randomGen,0.999);
}
void Fighter::HumanIntervention::control(){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        p->commands["weapon"]={
            {"launch",false},
            {"target",Track3D()}
        };
        if(p->launchable && p->commands["fromAgent"]["weapon"]["launch"]){
            if(recognizedShotCommands.size()<capacity
                && (recognizedShotCommands.size()==0 || manager->getTime()>=recognizedShotCommands.back().first+cooldown)
            ){
                recognizedShotCommands.push_back(std::make_pair(manager->getTime(),p->commands["fromAgent"]["weapon"]["target"]));
                if(recognizedShotCommands.size()>=capacity){
                    p->launchable=false;
                }
            }
        }
        if(recognizedShotCommands.size()>0){
            auto front=recognizedShotCommands.front();
            if(manager->getTime()>=front.first+delay){
                //承認
                p->commands["weapon"]={
                    {"launch",true},
                    {"target",front.second}
                };
                recognizedShotCommands.pop_front();
                if(recognizedShotCommands.size()<capacity){
                    p->launchable=true;
                }
            }else{
                //判断中
            }
        }
        p->commands["motion"]=p->commands["fromAgent"]["motion"];
    }
}
Fighter::WeaponController::WeaponController(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
}
void Fighter::WeaponController::control(){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        if(!p->commands.contains("weapon")){
            p->commands["weapon"]={
                {"launch",false},
                {"target",Track3D()}
            };
        }
        std::pair<bool,Track3D> trackingInfo=p->isTracking(p->commands["weapon"]["target"].get<Track3D>());
        p->target=trackingInfo.second;
        bool launchFlag=p->commands["weapon"]["launch"].get<bool>() && trackingInfo.first;
        if(p->remMsls==0){
            p->commands["weapon"]={
                {"launch",false},
                {"target",Track3D()}
            };
            p->launchable=false;
        }else if(launchFlag){
            p->missileTargets[p->nextMsl]=std::make_pair(p->target,true);
            p->communicationBuffers["MissileComm:"+p->missiles[p->nextMsl].lock()
                ->getFullName()].lock()->send({
                    {"launch",true},
                    {"target",p->missileTargets[p->nextMsl]}
                },CommunicationBuffer::MERGE);
            p->nextMsl+=1;
            p->remMsls-=1;
            manager->triggerEvent("Shot",{this->weak_from_this()});
            p->commands["weapon"]={
                {"launch",false},
                {"target",Track3D()}
            };
        }
        for(int mslId=0;mslId<p->nextMsl;++mslId){
            if(p->missileTargets[mslId].second){
                p->communicationBuffers["MissileComm:"+p->missiles[mslId].lock()
                ->getFullName()].lock()->send({
                    {"target",p->missileTargets[mslId].first}
                },CommunicationBuffer::MERGE);
            }
        }
    }
}
Fighter::FlightController::FlightController(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Controller(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    mode="direct";
}
Fighter::FlightController::~FlightController(){
}
void Fighter::FlightController::control(){
    auto p=getShared<Fighter>(parent);
    if(p->isAlive_){
        if(!p->commands.contains("motion")){
            p->commands["motion"]=getDefaultCommand();
        }
        commands["motion"]=calc(p->commands["motion"]);
    }
}
nl::json Fighter::FlightController::getDefaultCommand(){
    std::cout<<"Warning! Fighter::FlightController::getDefaultCommand() should be overridden."<<std::endl;
    return nl::json::object();
}
nl::json Fighter::FlightController::calc(const nl::json &cmd){
    std::cout<<"Warning! Fighter::FlightController::calc(const nl::json&) should be overridden."<<std::endl;
    return cmd;
}
void Fighter::FlightController::setMode(const std::string& ctrlName){
    mode=ctrlName;
}
FighterAccessor::FighterAccessor(std::shared_ptr<Fighter> a)
:PhysicalAssetAccessor(a){
    asset=a;
}
FighterAccessor::~FighterAccessor(){
}
void FighterAccessor::setFlightControllerMode(const std::string& ctrlName){
    asset.lock()->setFlightControllerMode(ctrlName);
}
double FighterAccessor::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt){
    return asset.lock()->getRmax(rs,vs,rt,vt);
}
double FighterAccessor::getRmax(const Eigen::Vector3d &rs,const Eigen::Vector3d &vs,const Eigen::Vector3d &rt,const Eigen::Vector3d &vt,const double& aa){
    return asset.lock()->getRmax(rs,vs,rt,vt,aa);
}

void exportFighter(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(Fighter)
    DEF_FUNC(Fighter,makeChildren)
    DEF_FUNC(Fighter,setDependency)
    DEF_FUNC(Fighter,perceive)
    DEF_FUNC(Fighter,control)
    DEF_FUNC(Fighter,behave)
    DEF_FUNC(Fighter,kill)
    .def("isTracking",py::overload_cast<std::weak_ptr<PhysicalAsset>>(&Fighter::isTracking))
    .def("isTracking",py::overload_cast<const Track3D&>(&Fighter::isTracking))
    DEF_FUNC(Fighter,setFlightControllerMode)
    DEF_FUNC(Fighter,calcMotion)
    DEF_FUNC(Fighter,toEulerAngle)
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&>(&Fighter::getRmax))
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>(&Fighter::getRmax))
    DEF_FUNC(Fighter,relHtoI)
    DEF_FUNC(Fighter,relItoH)
    DEF_FUNC(Fighter,absHtoI)
    DEF_FUNC(Fighter,absItoH)
    DEF_READWRITE(Fighter,rcsScale)
    DEF_READWRITE(Fighter,radar)
    DEF_READWRITE(Fighter,mws)
    DEF_READWRITE(Fighter,missiles)
    DEF_READWRITE(Fighter,nextMsl)
    DEF_READWRITE(Fighter,numMsls)
    DEF_READWRITE(Fighter,remMsls)
    DEF_READWRITE(Fighter,isDatalinkEnabled)
    DEF_READWRITE(Fighter,track)
    DEF_READWRITE(Fighter,trackSource)
    DEF_READWRITE(Fighter,launchable)
    DEF_READWRITE(Fighter,target)
    DEF_READWRITE(Fighter,targetID)
    ;
    EXPOSE_CLASS(Fighter::FlightController)
    DEF_FUNC(Fighter::FlightController,control)
    DEF_FUNC(Fighter::FlightController,getDefaultCommand)
    DEF_FUNC(Fighter::FlightController,calc)
    .def("calc",[](Fighter::FlightController& v,const py::object &cmd){
        return v.calc(cmd);
    })
    DEF_FUNC(Fighter::FlightController,setMode)
    ;
    EXPOSE_CLASS_WITHOUT_INIT(FighterAccessor)
    .def(py::init<std::shared_ptr<Fighter>>())
    DEF_FUNC(FighterAccessor,setFlightControllerMode)
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&>(&FighterAccessor::getRmax))
    .def("getRmax",py::overload_cast<const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const Eigen::Vector3d&,const double&>(&FighterAccessor::getRmax))
    ;
}
