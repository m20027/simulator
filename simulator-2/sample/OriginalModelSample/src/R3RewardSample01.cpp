#include "R3RewardSample01.h"
#include <algorithm>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/Units.h>
#include <ASRCAISim1/SimulationManager.h>
#include <ASRCAISim1/Asset.h>
#include <ASRCAISim1/Fighter.h>
#include <ASRCAISim1/Agent.h>
#include <ASRCAISim1/R3BVRRuler01.h>
using namespace util;

R3RewardSample01::R3RewardSample01(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:TeamReward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    pBite=getValueFromJsonKRD(modelConfig,"pBite",randomGen,2.0);
    pMemT=getValueFromJsonKRD(modelConfig,"pMemT",randomGen,0.2);
    pDetect=getValueFromJsonKRD(modelConfig,"pDetect",randomGen,0.001);
    pVel=getValueFromJsonKRD(modelConfig,"pVel",randomGen,0.0);
    pOmega=getValueFromJsonKRD(modelConfig,"pOmega",randomGen,0.0);
    pLine=getValueFromJsonKRD(modelConfig,"pLine",randomGen,0.0);
    pEnergy=getValueFromJsonKRD(modelConfig,"pEnergy",randomGen,2e-5);
}
R3RewardSample01::~R3RewardSample01(){}
void R3RewardSample01::onEpisodeBegin(){
    j_target="All";
    this->TeamReward::onEpisodeBegin();
    auto ruler_=getShared<Ruler,Ruler>(manager->getRuler());
    auto o=ruler_->observables;
    westSider=o.at("westSider");
    eastSider=o.at("eastSider");
    forwardAx=o.at("forwardAx").get<std::map<std::string,Eigen::Vector2d>>();
    friends.clear();
    enemies.clear();
    friendMsls.clear();
    numMissiles.clear();
    biteFlag.clear();
    memoryTrackFlag.clear();
    totalEnergy.clear();
    leadRangePrev.clear();
    leadRange.clear();
    for(auto& e:reward){
        std::string key=e.first;
        friends[key].clear();
        totalEnergy[key]=0;
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->getTeam()==key && isinstance<Fighter>(asset);
        })){
            auto f=getShared<Fighter>(e);
            friends[key].push_back(f);
			Eigen::Vector3d pos=f->posI();
			Eigen::Vector3d vel=f->velI();
            totalEnergy[key]+=vel.squaredNorm()/2-gravity*pos(2);
            if(leadRangePrev.count(key)>0){
                leadRangePrev[key]=std::max(leadRangePrev[key],forwardAx[key].dot(pos.block<2,1>(0,0,2,1)));
            }else{
                leadRangePrev[key]=forwardAx[key].dot(pos.block<2,1>(0,0,2,1));
            }
        }
        leadRange[key]=leadRangePrev[key];
        enemies[key].clear();
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->getTeam()!=key && isinstance<Fighter>(asset);
        })){
            auto f=getShared<Fighter>(e);
            enemies[key].push_back(f);
        }
        friendMsls[key].clear();
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->getTeam()==key && isinstance<Missile>(asset);
        })){
            auto m=getShared<Missile>(e);
            friendMsls[key].push_back(m);
        }
        numMissiles[key]=friendMsls[key].size();
        biteFlag[key]=VecX<bool>::Constant(numMissiles[key],false);
        memoryTrackFlag[key]=VecX<bool>::Constant(numMissiles[key],false);
    }
}
void R3RewardSample01::onStepEnd(){
    nl::json track=nl::json::array();
    for(auto& t:reward){
        std::string team=t.first;
		//(1)Biteへの加点、(2)メモリトラック落ちへの減点
        int i=0;
        for(auto&& m_:friendMsls[team]){
            auto m=m_.lock();
            if(m->hasLaunched && m->isAlive()){
                if(m->mode==Missile::Mode::SELF && !biteFlag[team](i)){
					reward[team]+=pBite;
					biteFlag[team](i)=true;
                }
				if(m->mode==Missile::Mode::MEMORY && !memoryTrackFlag[team](i)){
					reward[team]-=pMemT;
					memoryTrackFlag[team](i)=true;
                }
            }
            ++i;
        }
		//(3)敵探知への加点(生存中の敵の何%を探知できているか)(DL前提)
        track=nl::json::array();
        for(auto&& f_:friends[team]){
            if(f_.lock()->isAlive()){
                track=f_.lock()->observables.at("/sensor/track"_json_pointer);
                break;
            }
        }
        int numAlive=0;
        int numTracked=0;
        for(auto&& f_:enemies[team]){
            auto f=f_.lock();
			if(f->isAlive()){
				numAlive+=1;
                for(auto&& t_:track){
                    if(t_.get<Track3D>().isSame(f)){
					    numTracked+=1;
                        break;
                    }
                }
            }
        }
		if(numAlive>0){
			reward[team]+=(1.0*numTracked/numAlive)*pDetect;
        }
        double ene=0;
        std::vector<double> tmp;
        tmp.push_back(-std::numeric_limits<double>::infinity());
        for(auto&& f_:friends[team]){
            auto f=f_.lock();
			Eigen::Vector3d pos=f->posI();
			Eigen::Vector3d vel=f->velI();
			Eigen::Vector3d omega=f->omegaI();
			//(4)過剰な機動への減点(角速度ノルムに対してL2、高度方向速度に対してL1正則化)
			reward[team]+=-pVel*abs(vel(2))-(omega.squaredNorm())*pOmega;
			//(5)前進・後退への更なる加減点
            tmp.push_back(forwardAx[team].dot(f->posI().block<2,1>(0,0,2,1)));
            //(6)保持している力学的エネルギー(回転を除く)の増減による加減点
            ene+=vel.squaredNorm()/2-gravity*pos(2);
        }
        leadRange[team]=*std::max_element(tmp.begin(),tmp.end());
		reward[team]+=(leadRange[team]-leadRangePrev[team])*pLine;
        leadRangePrev[team]=leadRange[team];
        reward[team]+=(ene-totalEnergy[team])*pEnergy;
        totalEnergy[team]=ene;
    }
    this->TeamReward::onStepEnd();
}

void exportR3RewardSample01(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(R3RewardSample01)
    DEF_FUNC(R3RewardSample01,onEpisodeBegin)
    DEF_FUNC(R3RewardSample01,onStepEnd)
    DEF_READWRITE(R3RewardSample01,pBite)
    DEF_READWRITE(R3RewardSample01,pMemT)
    DEF_READWRITE(R3RewardSample01,pDetect)
    DEF_READWRITE(R3RewardSample01,pVel)
    DEF_READWRITE(R3RewardSample01,pOmega)
    DEF_READWRITE(R3RewardSample01,pLine)
    DEF_READWRITE(R3RewardSample01,pEnergy)
    DEF_READWRITE(R3RewardSample01,westSider)
    DEF_READWRITE(R3RewardSample01,eastSider)
    DEF_READWRITE(R3RewardSample01,forwardAx)
    DEF_READWRITE(R3RewardSample01,numMissiles)
    DEF_READWRITE(R3RewardSample01,biteFlag)
    DEF_READWRITE(R3RewardSample01,memoryTrackFlag)
    DEF_READWRITE(R3RewardSample01,friends)
    DEF_READWRITE(R3RewardSample01,enemies)
    DEF_READWRITE(R3RewardSample01,friendMsls)
    ;
}
