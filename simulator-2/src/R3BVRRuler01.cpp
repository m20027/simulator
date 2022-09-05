#include "R3BVRRuler01.h"
#include <algorithm>
#include "Utility.h"
#include "SimulationManager.h"
#include "Asset.h"
#include "Fighter.h"
#include "Agent.h"
using namespace util;

R3BVRRuler01::R3BVRRuler01(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:Ruler(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    dLine=getValueFromJsonKRD(modelConfig,"dLine",randomGen,100000);
    dOut=getValueFromJsonKRD(modelConfig,"dOut",randomGen,75000);
    hLim=getValueFromJsonKRD(modelConfig,"hLim",randomGen,20000);
    westSider=getValueFromJsonKRD<std::string>(modelConfig,"westSider",randomGen,"Red");
    eastSider=getValueFromJsonKRD<std::string>(modelConfig,"eastSider",randomGen,"Blue");
    pDisq=getValueFromJsonKRD(modelConfig,"pDisq",randomGen,-100);
    pBreak=getValueFromJsonKRD(modelConfig,"pBreak",randomGen,10);
    pDown=getValueFromJsonKRD(modelConfig,"pDown",randomGen,5);
    pAdv=getValueFromJsonKRD(modelConfig,"pAdv",randomGen,0.1);
    pOut=getValueFromJsonKRD(modelConfig,"pOut",randomGen,0.1);
    crashCount=std::map<std::string,int>();
    hitCount=std::map<std::string,int>();
    forwardAx=std::map<std::string,Eigen::Vector2d>();
    sideAx=std::map<std::string,Eigen::Vector2d>();
    endReason=EndReason::NOTYET;
    endReasonSub=EndReason::NOTYET;
}
R3BVRRuler01::~R3BVRRuler01(){}
void R3BVRRuler01::onEpisodeBegin(){
    modelConfig["teams"]=nl::json::array({westSider,eastSider});
    this->Ruler::onEpisodeBegin();
    assert(score.size()==2);
    assert(score.count(westSider)==1 && score.count(eastSider)==1);
    manager->addEventHandler("Crash",[&](const nl::json& args){this->R3BVRRuler01::onCrash(args);});//墜落数監視用
    manager->addEventHandler("Hit",[&](const nl::json& args){this->R3BVRRuler01::onHit(args);});//撃墜数監視用
    crashCount.clear();
    hitCount.clear();
    leadRange.clear();
    lastDownPosition.clear();
    lastDownReason.clear();
    outDist.clear();
    breakTime.clear();
    disqTime.clear();
    forwardAx.clear();
    sideAx.clear();
    forwardAx[westSider]=Eigen::Vector2d(0.,1.);
    forwardAx[eastSider]=Eigen::Vector2d(0.,-1.);
    sideAx[westSider]=Eigen::Vector2d(-1.,0.);
    sideAx[eastSider]=Eigen::Vector2d(1.,0.);
    for(auto& t:teams){
        crashCount[t]=0;
        hitCount[t]=0;
        leadRange[t]=-dLine;
        outDist[t]=0;
        eliminatedTime[t]=-1;
        breakTime[t]=-1;
        disqTime[t]=-1;
    }
    endReason=EndReason::NOTYET;
    endReasonSub=EndReason::NOTYET;
    observables={
        {"eastSider",eastSider},
        {"westSider",westSider},
        {"dOut",dOut},
        {"dLine",dLine},
        {"hLim",hLim},
        {"forwardAx",forwardAx},
        {"sideAx",sideAx},
        {"endReason",enumToJson(endReason)}
    };
}
void R3BVRRuler01::onCrash(const nl::json& args){
    std::shared_ptr<PhysicalAsset> asset=args;
    crashCount[asset->getTeam()]+=1;
    lastDownPosition[asset->getTeam()]=forwardAx[asset->getTeam()].dot(asset->posI().block<2,1>(0,0,2,1));
    lastDownReason[asset->getTeam()]=DownReason::CRASH;
}
void R3BVRRuler01::onHit(const nl::json& args){//{"wpn":wpn,"tgt":tgt}
    std::shared_ptr<PhysicalAsset> wpn=args.at("wpn");
    std::shared_ptr<PhysicalAsset> tgt=args.at("tgt");
    hitCount[wpn->getTeam()]+=1;
    lastDownPosition[tgt->getTeam()]=forwardAx[tgt->getTeam()].dot(tgt->posI().block<2,1>(0,0,2,1));
    lastDownReason[tgt->getTeam()]=DownReason::HIT;
}
void R3BVRRuler01::onInnerStepBegin(){
    std::vector<double> tmp;
    for(auto&& t:teams){
        crashCount[t]=0;
        hitCount[t]=0;
        if(manager->getTickCount()==0){
            tmp.clear();
            tmp.push_back(-dLine);
            for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
                return asset->isAlive() && asset->getTeam()==t && isinstance<Fighter>(asset);
            })){
                auto f=getShared<Fighter>(e);
                tmp.push_back(forwardAx[t].dot(f->posI().block<2,1>(0,0,2,1)));
            }
            leadRange[t]=*std::max_element(tmp.begin(),tmp.end());
        }
    }
}
void R3BVRRuler01::onInnerStepEnd(){
    for(auto& team:teams){
        //得点計算 1：撃墜による加点(1機あたりpDown点)
		stepScore[team]+=hitCount[team]*pDown;
	    //得点計算 4-(a)：墜落による減点(1機あたりpDown点)
		stepScore[team]-=crashCount[team]*pDown;
        //得点計算 4-(b)：場外に対する減点(1秒、1kmにつきpOut点)
        outDist[team]=0;
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->isAlive() && asset->getTeam()==team && isinstance<Fighter>(asset);
        })){
            auto f=getShared<Fighter>(e);
            outDist[team]+=std::max(0.0,abs(sideAx[team].dot(f->posI().block<2,1>(0,0,2,1)))-dOut);
        }
        stepScore[team]-=(outDist[team]/1000.)*pOut*manager->getBaseTimeStep();
        if(score[team]+stepScore[team]<=pDisq && disqTime[team]<0){
            disqTime[team]=manager->getTime();
        }
    }
    //生存数の監視
    std::map<std::string,int> aliveCount;
    for(auto& team:teams){
        aliveCount[team]=0;
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->isAlive() && asset->getTeam()==team && isinstance<Fighter>(asset);
        })){
            aliveCount[team]++;
        }
        if(aliveCount[team]==0 && eliminatedTime[team]<0){
            eliminatedTime[team]=manager->getTime();
        }
    }
    //防衛ラインの突破タイミングを監視
    std::vector<double> tmp;
    for(auto& team:teams){
        tmp.clear();
        tmp.push_back(-dLine);
        for(auto&& e:manager->getAssets([&](std::shared_ptr<const Asset> asset)->bool{
            return asset->isAlive() && asset->getTeam()==team && isinstance<Fighter>(asset);
        })){
            auto f=getShared<Fighter>(e);
            tmp.push_back(forwardAx[team].dot(f->posI().block<2,1>(0,0,2,1)));
        }
        leadRange[team]=*std::max_element(tmp.begin(),tmp.end());
        if(leadRange[team]>=dLine && breakTime[team]<0){
            breakTime[team]=manager->getTime();
        }
    }
    if(endReasonSub==EndReason::NOTYET){
        if(eliminatedTime[westSider]>=0 || eliminatedTime[eastSider]>=0){
            endReasonSub=EndReason::ELIMINATION;
        }else if(breakTime[westSider]>=0 || breakTime[eastSider]>=0){
            endReasonSub=EndReason::BREAK;
        }else if(disqTime[westSider]>=0 || disqTime[eastSider]>=0){
            endReasonSub=EndReason::PENALTY;
        }
    }
}
void R3BVRRuler01::checkDone(){
	//終了判定
    dones.clear();
    for(auto&& e:manager->getAgents()){
        auto a=getShared(e);
        dones[a->getName()]=!a->isAlive();
    }
	//終了条件(1)：全機撃墜or墜落
    if(endReasonSub==EndReason::ELIMINATION){
        if(eliminatedTime[westSider]==eliminatedTime[eastSider]){
            //同時なら(3)扱い
            endReason=EndReason::TIMEUP;
            for(auto&& t:teams){
                leadRange[t]=lastDownPosition[t];
            }
        }else{
            endReason=EndReason::ELIMINATION;
        }
    }
    //終了条件(2)：防衛ラインの突破
    else if(endReasonSub==EndReason::BREAK){
        if(breakTime[westSider]==breakTime[eastSider]){
            //同時なら(3)扱い
            endReason=EndReason::TIMEUP;
        }else{
            endReason=EndReason::BREAK;
        }
    }
    //終了条件(4)：ペナルティによる敗北
    else if(endReasonSub==EndReason::PENALTY){
        if(disqTime[westSider]==disqTime[eastSider]){
            //同時なら(3)扱い
            endReason=EndReason::TIMEUP;
        }else{
            endReason=EndReason::PENALTY;
        }
    }
    //終了条件(3)：時間切れ
    else if(manager->getTime()>=maxTime){
        endReason=EndReason::TIMEUP;
    }
    //終了条件に応じた得点計算
    if(endReason==EndReason::ELIMINATION){
        //終了条件(1)：全機撃墜or墜落
        if(eliminatedTime[westSider]>=0 && eliminatedTime[eastSider]>=0){
            //ステップ終了時点で両者全滅していた場合
            //最後の撃墜or墜落による得点増減を無効化する
            if(eliminatedTime[westSider]>eliminatedTime[eastSider]){//東が先に全滅
                if(lastDownReason[westSider]==DownReason::CRASH){
                    //墜落の場合は減点を無効化
    		        stepScore[westSider]+=pDown;
	    	        score[westSider]+=pDown;
                }else{//HIT
                    //撃墜の場合は相手の加点を無効化
    		        stepScore[eastSider]-=pDown;
	    	        score[eastSider]-=pDown;
                }
            }else{//西が先に全滅
                if(lastDownReason[eastSider]==DownReason::CRASH){
                    //墜落の場合は減点を無効化
    		        stepScore[eastSider]+=pDown;
	    	        score[eastSider]+=pDown;
                }else{//HIT
                    //撃墜の場合は相手の加点を無効化
    		        stepScore[westSider]-=pDown;
	    	        score[westSider]-=pDown;
                }
            }
        }
    }else if(endReason==EndReason::BREAK){
        //終了条件(2)：防衛ラインの突破
        //得点計算 2：突破した陣営に+pBreak点
        if(breakTime[westSider]<0){//東だけが突破
		    stepScore[eastSider]+=pBreak;
		    score[eastSider]+=pBreak;
        }else if(breakTime[eastSider]<0){//西だけが突破
		    stepScore[westSider]+=pBreak;
		    score[westSider]+=pBreak;
        }else{//ステップ終了時点で両者突破
            if(breakTime[westSider]>breakTime[eastSider]){//東が先に突破
		        stepScore[eastSider]+=pBreak;
		        score[eastSider]+=pBreak;
            }else{
		        stepScore[westSider]+=pBreak;
		        score[westSider]+=pBreak;
            }
        }
    }
    if(endReason!=EndReason::NOTYET && endReason!=EndReason::BREAK){
        //終了条件(2)以外
        //得点計算 3：進出度合いに対する加点(1kmにつきpAdv点)
        if(leadRange[westSider]>leadRange[eastSider]){
            double s=(leadRange[westSider]-leadRange[eastSider])/2./1000.*pAdv;
		    stepScore[westSider]+=s;
		    score[westSider]+=s;
        }else{
            double s=(leadRange[eastSider]-leadRange[westSider])/2./1000.*pAdv;
		    stepScore[eastSider]+=s;
		    score[eastSider]+=s;
        }
    }
    //打ち切り対策で常時勝者を仮設定
    if(score[westSider]>score[eastSider]){
        winner=westSider;
    }else if(score[westSider]<score[eastSider]){
        winner=eastSider;
    }else{
        winner="";
    }
    if(endReason!=EndReason::NOTYET){
        for(auto& e:dones){
            e.second=true;
        }
        dones["__all__"]=true;
    }else{
        dones["__all__"]=false;
    }
    observables["endReason"]=enumToJson(endReason);
}

void exportR3BVRRuler01(py::module& m)
{
    using namespace pybind11::literals;
    BIND_MAP_NAME(std::string,R3BVRRuler01::DownReason,"std::map<std::string,R3BVRRuler01::DownReason>",false);

    auto cls=EXPOSE_CLASS(R3BVRRuler01);
    cls
    DEF_FUNC(R3BVRRuler01,onCrash)
    .def("onCrash",[](R3BVRRuler01& v,const py::object &args){
        return v.onCrash(args);
    })
    DEF_FUNC(R3BVRRuler01,onHit)
    .def("onHit",[](R3BVRRuler01& v,const py::object &args){
        return v.onHit(args);
    })
    DEF_FUNC(R3BVRRuler01,onEpisodeBegin)
    DEF_FUNC(R3BVRRuler01,onInnerStepBegin)
    DEF_FUNC(R3BVRRuler01,onInnerStepEnd)
    DEF_FUNC(R3BVRRuler01,checkDone)
    DEF_READWRITE(R3BVRRuler01,dLine)
    DEF_READWRITE(R3BVRRuler01,dOut)
    DEF_READWRITE(R3BVRRuler01,hLim)
    DEF_READWRITE(R3BVRRuler01,westSider)
    DEF_READWRITE(R3BVRRuler01,eastSider)
    DEF_READWRITE(R3BVRRuler01,pDisq)
    DEF_READWRITE(R3BVRRuler01,pBreak)
    DEF_READWRITE(R3BVRRuler01,pDown)
    DEF_READWRITE(R3BVRRuler01,pAdv)
    DEF_READWRITE(R3BVRRuler01,pOut)
    DEF_READWRITE(R3BVRRuler01,crashCount)
    DEF_READWRITE(R3BVRRuler01,hitCount)
    DEF_READWRITE(R3BVRRuler01,leadRange)
    DEF_READWRITE(R3BVRRuler01,lastDownPosition)
    DEF_READWRITE(R3BVRRuler01,lastDownReason)
    DEF_READWRITE(R3BVRRuler01,outDist)
    DEF_READWRITE(R3BVRRuler01,breakTime)
    DEF_READWRITE(R3BVRRuler01,disqTime)
    DEF_READWRITE(R3BVRRuler01,forwardAx)
    DEF_READWRITE(R3BVRRuler01,sideAx)
    DEF_READWRITE(R3BVRRuler01,endReason)
    DEF_READWRITE(R3BVRRuler01,endReasonSub)
    ;
    py::enum_<R3BVRRuler01::DownReason>(cls,"DownReason")
    .value("CRASH",R3BVRRuler01::DownReason::CRASH)
    .value("HIT",R3BVRRuler01::DownReason::HIT)
    ;
    py::enum_<R3BVRRuler01::EndReason>(cls,"EndReason")
    .value("NOTYET",R3BVRRuler01::EndReason::NOTYET)
    .value("ELIMINATION",R3BVRRuler01::EndReason::ELIMINATION)
    .value("BREAK",R3BVRRuler01::EndReason::BREAK)
    .value("TIMEUP",R3BVRRuler01::EndReason::TIMEUP)
    .value("PENALTY",R3BVRRuler01::EndReason::PENALTY)
    ;
}