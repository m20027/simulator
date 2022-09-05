#include "R3BVRBasicReward01.h"
#include <algorithm>
#include "Utility.h"
#include "SimulationManager.h"
#include "Asset.h"
#include "Fighter.h"
#include "Agent.h"
using namespace util;

R3BVRBasicReward01::R3BVRBasicReward01(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:TeamReward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    rElim=getValueFromJsonKRD(modelConfig,"rElim",randomGen,0);
    rElimE=getValueFromJsonKRD(modelConfig,"rElimE",randomGen,0);
    rBreakRatio=getValueFromJsonKRD(modelConfig,"rBreakRatio",randomGen,0);
    rBreak=getValueFromJsonKRD(modelConfig,"rBreak",randomGen,0);
    rBreakE=getValueFromJsonKRD(modelConfig,"rBreakE",randomGen,0);
    adjustBreakEnd=getValueFromJsonKRD(modelConfig,"adjustBreakEnd",randomGen,true);
    rTimeup=getValueFromJsonKRD(modelConfig,"rTimeup",randomGen,0);
    rDisq=getValueFromJsonKRD(modelConfig,"rDisq",randomGen,0);
    rDisqE=getValueFromJsonKRD(modelConfig,"rDisqE",randomGen,0);
    rHitRatio=getValueFromJsonKRD(modelConfig,"rHitRatio",randomGen,0);
    rHit=getValueFromJsonKRD(modelConfig,"rHit",randomGen,0);
    rHitE=getValueFromJsonKRD(modelConfig,"rHitE",randomGen,0);
    rAdvRatio=getValueFromJsonKRD(modelConfig,"rAdvRatio",randomGen,0);
    acceptNegativeAdv=getValueFromJsonKRD(modelConfig,"acceptNegativeAdv",randomGen,true);
    rCrashRatio=getValueFromJsonKRD(modelConfig,"rCrashRatio",randomGen,0);
    rCrash=getValueFromJsonKRD(modelConfig,"rCrash",randomGen,0);
    rCrashE=getValueFromJsonKRD(modelConfig,"rCrashE",randomGen,0);
    rOutRatio=getValueFromJsonKRD(modelConfig,"rOutRatio",randomGen,0);
    adjustZerosum=getValueFromJsonKRD(modelConfig,"adjustZerosum",randomGen,true);
    crashCount=std::map<std::string,int>();
    hitCount=std::map<std::string,int>();
}
R3BVRBasicReward01::~R3BVRBasicReward01(){}
void R3BVRBasicReward01::onEpisodeBegin(){
    ruler=getShared<R3BVRRuler01>(manager->getRuler());
    assert(ruler);
    this->TeamReward::onEpisodeBegin();
    assert(reward.size()<=2);
    for(auto&& t:reward){
        assert(t.first==ruler->westSider || t.first==ruler->eastSider);
    }
    opponentName.clear();
    opponentName[ruler->westSider]=ruler->eastSider;
    opponentName[ruler->eastSider]=ruler->westSider;
    manager->addEventHandler("Crash",[&](const nl::json& args){this->R3BVRBasicReward01::onCrash(args);});//墜落数監視用
    manager->addEventHandler("Hit",[&](const nl::json& args){this->R3BVRBasicReward01::onHit(args);});//撃墜数監視用
    crashCount.clear();
    hitCount.clear();
    advPrev.clear();
    eliminatedTime.clear();
    breakTime.clear();
    disqTime.clear();
    std::vector<double> tmp;
    for(auto& t:ruler->teams){
        crashCount[t]=0;
        hitCount[t]=0;
        if(adjustZerosum){
            advPrev[t]=(ruler->leadRange[t]-ruler->leadRange[opponentName[t]])/2;
        }else{
            advPrev[t]=std::max(0.0,ruler->leadRange[t]-ruler->leadRange[opponentName[t]])/2;
        }
        eliminatedTime[t]=-1;
        breakTime[t]=-1;
        disqTime[t]=-1;
    }
}
void R3BVRBasicReward01::onCrash(const nl::json& args){
    std::shared_ptr<PhysicalAsset> asset=args;
    crashCount[asset->getTeam()]+=1;
}
void R3BVRBasicReward01::onHit(const nl::json& args){//{"wpn":wpn,"tgt":tgt}
    std::shared_ptr<PhysicalAsset> wpn=args.at("wpn");
    hitCount[wpn->getTeam()]+=1;
}
void R3BVRBasicReward01::onInnerStepBegin(){
    for(auto&& t:ruler->teams){
        crashCount[t]=0;
        hitCount[t]=0;
        advPrev[t]=(ruler->leadRange[t]-ruler->leadRange[opponentName[t]])/2;
        if(!acceptNegativeAdv){
            advPrev[t]=std::max(0.0,advPrev[t]);
        }else if(adjustZerosum){
            advPrev[t]/=2;
        }
    }
}
void R3BVRBasicReward01::onInnerStepEnd(){
    for(auto& team:target){
    	reward[team]-=crashCount[team]*ruler->pDown*(1+rCrashRatio);
        reward[team]+=crashCount[team]*rCrash;
        reward[team]+=crashCount[opponentName[team]]*rCrashE;
    	reward[team]+=hitCount[team]*ruler->pDown*(1+rHitRatio);
        reward[team]+=hitCount[team]*rHit;
        reward[team]+=hitCount[opponentName[team]]*rHitE;
        reward[team]-=(ruler->outDist[team]/1000.)*ruler->pOut*manager->getBaseTimeStep()*(1+rOutRatio);
    }
    for(auto& team:ruler->teams){
        if(ruler->eliminatedTime[team]>=0 && eliminatedTime[team]<0){
            if(reward.count(team)>0){
                reward[team]+=rElimE;
            }
            if(reward.count(opponentName[team])>0){
                reward[opponentName[team]]+=rElim;
            }
            eliminatedTime[team]=manager->getTime();
        }
        if(ruler->breakTime[team]>=0 && breakTime[team]<0){
            //追加の報酬
            if(reward.count(team)>0){
                reward[team]+=rBreak;
            }
            if(reward.count(opponentName[team])>0){
                reward[opponentName[team]]+=rBreakE;
            }
            breakTime[team]=manager->getTime();
        }
        if(ruler->disqTime[team]>=0 && disqTime[team]<0){
            if(reward.count(team)>0){
                reward[team]+=rDisq;
            }
            if(reward.count(opponentName[team])>0){
                reward[opponentName[team]]+=rDisqE;
            }
            disqTime[team]=manager->getTime();
        }
    }
    if(reward.count(ruler->westSider)>0){
        double adv=(ruler->leadRange[ruler->westSider]-ruler->leadRange[ruler->eastSider])/2;
        if(!acceptNegativeAdv){
            adv=std::max(0.0,adv);
        }else if(adjustZerosum){
            adv/=2;
        }
        reward[ruler->westSider]+=(adv-advPrev[ruler->westSider])/1000.*ruler->pAdv*(1+rAdvRatio);
    }
    if(reward.count(ruler->eastSider)>0){
        double adv=(ruler->leadRange[ruler->eastSider]-ruler->leadRange[ruler->westSider])/2;
        if(!acceptNegativeAdv){
            adv=std::max(0.0,adv);
        }else if(adjustZerosum){
            adv/=2;
        }
        reward[ruler->eastSider]+=(adv-advPrev[ruler->eastSider])/1000.*ruler->pAdv*(1+rAdvRatio);
    }
}
void R3BVRBasicReward01::onStepEnd(){
    if(ruler->endReason!=R3BVRRuler01::EndReason::NOTYET){
        //終了時の帳尻合わせ
        if(ruler->endReason==R3BVRRuler01::EndReason::ELIMINATION){
            //終了条件(1)：全機撃墜or墜落
            if(ruler->eliminatedTime[ruler->westSider]==0 && ruler->eliminatedTime[ruler->eastSider]==0){
                //ステップ終了時点で両者全滅していた場合
                //最後の撃墜or墜落による報酬を無効化する
                if(ruler->eliminatedTime[ruler->westSider]>ruler->eliminatedTime[ruler->eastSider]){//東が先に全滅
                    if(ruler->lastDownReason[ruler->westSider]==R3BVRRuler01::DownReason::CRASH){
                        //墜落の場合
                        if(reward.count(ruler->westSider)>0){
    		                reward[ruler->westSider]+=ruler->pDown*(1+rCrashRatio);
    		                reward[ruler->westSider]-=rCrash;
                        }
                        if(reward.count(ruler->eastSider)>0){
    		                reward[ruler->eastSider]-=rCrashE;
                        }
                    }else{//HIT
                        //撃墜の場合
                        if(reward.count(ruler->eastSider)>0){
    		                reward[ruler->eastSider]-=ruler->pDown*(1+rHitRatio);
    		                reward[ruler->eastSider]-=rHit;
                        }
                        if(reward.count(ruler->westSider)>0){
    		                reward[ruler->westSider]-=rHitE;
                        }
                    }
                }else{//西が先に全滅
                    if(ruler->lastDownReason[ruler->eastSider]==R3BVRRuler01::DownReason::CRASH){
                        //墜落の場合
                        if(reward.count(ruler->eastSider)>0){
    		                reward[ruler->eastSider]+=ruler->pDown*(1+rCrashRatio);
    		                reward[ruler->eastSider]-=rCrash;
                        }
                        if(reward.count(ruler->westSider)>0){
    		                reward[ruler->westSider]-=rCrashE;
                        }
                    }else{//HIT
                        //撃墜の場合
                        if(reward.count(ruler->westSider)>0){
    		                reward[ruler->westSider]-=ruler->pDown*(1+rHitRatio);
    		                reward[ruler->westSider]-=rHit;
                        }
                        if(reward.count(ruler->eastSider)>0){
    		                reward[ruler->eastSider]-=rHitE;
                        }
                    }
                }
            }
        }else if(ruler->endReason==R3BVRRuler01::EndReason::BREAK){
            //終了条件(2)：防衛ラインの突破
            if(adjustBreakEnd){
                //進出度合いによる報酬を無効化する
                if(reward.count(ruler->westSider)>0){
                    reward[ruler->westSider]-=advPrev[ruler->westSider]/1000.*ruler->pAdv*(1+rAdvRatio);
                }
                if(reward.count(ruler->eastSider)>0){
                    reward[ruler->eastSider]-=advPrev[ruler->eastSider]/1000.*ruler->pAdv*(1+rAdvRatio);
                }
            }
            if(ruler->breakTime[ruler->westSider]<0){//東だけが突破
                if(reward.count(ruler->eastSider)>0){
		            reward[ruler->eastSider]+=ruler->pBreak*(1+rBreakRatio);
                }
            }else if(ruler->breakTime[ruler->eastSider]<0){//西だけが突破
                if(reward.count(ruler->westSider)>0){
    		        reward[ruler->westSider]+=ruler->pBreak*(1+rBreakRatio);
                }
            }else{//ステップ終了時点で両者突破
                if(ruler->breakTime[ruler->westSider]>ruler->breakTime[ruler->eastSider]){//東が先に突破
                    if(reward.count(ruler->eastSider)>0){
		                reward[ruler->eastSider]+=ruler->pBreak*(1+rBreakRatio);
                    }
                }else{
                    if(reward.count(ruler->westSider)>0){
		                reward[ruler->westSider]+=ruler->pBreak*(1+rBreakRatio);
                    }
                }
            }
        }else if(ruler->endReason==R3BVRRuler01::EndReason::TIMEUP){
            //終了条件(3)：時間切れ
            //何もしない
        }
        if(ruler->endReason!=R3BVRRuler01::EndReason::NOTYET){
            if(manager->getTime()>=ruler->maxTime){
                for(auto&& team:target){
                    reward[team]+=rTimeup;
                }
            }
        }
    }
    if(adjustZerosum){
        std::map<std::string,double> buffer;
        for(auto&& t:target){
            buffer[t]=reward[t];
        }
        for(auto&& t:target){
            reward[t]-=buffer[opponentName[t]];
        }
    }
    this->TeamReward::onStepEnd();
}

void exportR3BVRBasicReward01(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(R3BVRBasicReward01)
    DEF_FUNC(R3BVRBasicReward01,onCrash)
    .def("onCrash",[](R3BVRBasicReward01& v,const py::object &args){
        return v.onCrash(args);
    })
    DEF_FUNC(R3BVRBasicReward01,onHit)
    .def("onHit",[](R3BVRBasicReward01& v,const py::object &args){
        return v.onHit(args);
    })
    DEF_FUNC(R3BVRBasicReward01,onEpisodeBegin)
    DEF_FUNC(R3BVRBasicReward01,onInnerStepBegin)
    DEF_FUNC(R3BVRBasicReward01,onInnerStepEnd)
    DEF_FUNC(R3BVRBasicReward01,onStepEnd)
    DEF_READWRITE(R3BVRBasicReward01,rElim)
    DEF_READWRITE(R3BVRBasicReward01,rElimE)
    DEF_READWRITE(R3BVRBasicReward01,rBreakRatio)
    DEF_READWRITE(R3BVRBasicReward01,rBreak)
    DEF_READWRITE(R3BVRBasicReward01,rBreakE)
    DEF_READWRITE(R3BVRBasicReward01,adjustBreakEnd)
    DEF_READWRITE(R3BVRBasicReward01,rTimeup)
    DEF_READWRITE(R3BVRBasicReward01,rDisq)
    DEF_READWRITE(R3BVRBasicReward01,rDisqE)
    DEF_READWRITE(R3BVRBasicReward01,rHitRatio)
    DEF_READWRITE(R3BVRBasicReward01,rHit)
    DEF_READWRITE(R3BVRBasicReward01,rHitE)
    DEF_READWRITE(R3BVRBasicReward01,rAdvRatio)
    DEF_READWRITE(R3BVRBasicReward01,acceptNegativeAdv)
    DEF_READWRITE(R3BVRBasicReward01,rCrashRatio)
    DEF_READWRITE(R3BVRBasicReward01,rCrash)
    DEF_READWRITE(R3BVRBasicReward01,rCrashE)
    DEF_READWRITE(R3BVRBasicReward01,rOutRatio)
    DEF_READWRITE(R3BVRBasicReward01,adjustZerosum)
    DEF_READWRITE(R3BVRBasicReward01,crashCount)
    DEF_READWRITE(R3BVRBasicReward01,hitCount)
    DEF_READWRITE(R3BVRBasicReward01,eliminatedTime)
    DEF_READWRITE(R3BVRBasicReward01,breakTime)
    DEF_READWRITE(R3BVRBasicReward01,disqTime)
    ;
}