#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <ASRCAISim1/MathUtility.h>
#include <ASRCAISim1/Utility.h>
#include <ASRCAISim1/Reward.h>
#include <ASRCAISim1/Fighter.h>
#include <ASRCAISim1/Missile.h>
namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITHOUT_TRAMPOLINE(R3RewardSample01,TeamReward)
	/*いくつかの観点に基づいた報酬の実装例。
	(1)Bite(誘導弾シーカで目標を捕捉)への加点
	(2)誘導弾目標のメモリトラック落ちへの減点
	(3)敵探知への加点(生存中の敵の何%を探知できているか)
	(4)過剰な機動への減点
	(5)前進・後退への更なる加減点
    (6)保持している力学的エネルギー(回転を除く)の多寡による加減点
	*/
    public:
    //parameters
    double pBite,pMemT,pDetect,pVel,pOmega,pLine,pEnergy;
    //internal variables
    std::string westSider,eastSider;
    std::map<std::string,double> leadRange;
    std::map<std::string,double> leadRangePrev;
    std::map<std::string,Eigen::Vector2d> forwardAx;
    std::map<std::string,std::size_t> numMissiles;
    std::map<std::string,VecX<bool>> biteFlag,memoryTrackFlag;
    std::map<std::string,std::vector<std::weak_ptr<Fighter>>> friends,enemies;
    std::map<std::string,std::vector<std::weak_ptr<Missile>>> friendMsls;
    std::map<std::string,double> totalEnergy;
    //constructors & destructor
    R3RewardSample01(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~R3RewardSample01();
    //functions
    virtual void onEpisodeBegin();
    virtual void onStepEnd();
};
void exportR3RewardSample01(py::module& m);
