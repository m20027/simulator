#include "WinLoseReward.h"
#include <algorithm>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/Units.h>
#include <ASRCAISim1/SimulationManager.h>
#include <ASRCAISim1/Asset.h>
#include <ASRCAISim1/Fighter.h>
#include <ASRCAISim1/Agent.h>
#include <ASRCAISim1/R3BVRRuler01.h>
using namespace util;

WinLoseReward::WinLoseReward(const nl::json& modelConfig_,const nl::json& instanceConfig_)
:TeamReward(modelConfig_,instanceConfig_){
    if(isDummy){return;}
    win=getValueFromJsonKRD(modelConfig,"win",randomGen,1.0);
    lose=getValueFromJsonKRD(modelConfig,"lose",randomGen,-1.0);
}
WinLoseReward::~WinLoseReward(){}
void WinLoseReward::onEpisodeBegin(){
    j_target="All";
    this->TeamReward::onEpisodeBegin();
    auto ruler_=getShared<Ruler,Ruler>(manager->getRuler());
    auto o=ruler_->observables;
    westSider=o.at("westSider");
    eastSider=o.at("eastSider");
}
void WinLoseReward::onStepEnd(){
    auto ruler_=getShared<Ruler,Ruler>(manager->getRuler());
    if(ruler_->dones["__all__"]){
        if(ruler_->winner==westSider){
            reward[westSider]+=win;
            reward[eastSider]+=lose;
        }else{
            reward[eastSider]+=win;
            reward[westSider]+=lose;
        }
    }
    this->TeamReward::onStepEnd();
}

void exportWinLoseReward(py::module& m)
{
    using namespace pybind11::literals;
    EXPOSE_CLASS(WinLoseReward)
    DEF_FUNC(WinLoseReward,onEpisodeBegin)
    DEF_FUNC(WinLoseReward,onStepEnd)
    DEF_READWRITE(WinLoseReward,win)
    DEF_READWRITE(WinLoseReward,lose)
    ;
}
