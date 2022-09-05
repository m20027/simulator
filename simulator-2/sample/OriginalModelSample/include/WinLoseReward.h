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

DECLARE_CLASS_WITHOUT_TRAMPOLINE(WinLoseReward,TeamReward)
	/*得点差や途中経過によらず、終了時の勝ち負けのみによる報酬を与える例
	*/
    public:
    //parameters
    double win,lose;
    //internal variables
    std::string westSider,eastSider;
    //constructors & destructor
    WinLoseReward(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~WinLoseReward();
    //functions
    virtual void onEpisodeBegin();
    virtual void onStepEnd();
};
void exportWinLoseReward(py::module& m);
