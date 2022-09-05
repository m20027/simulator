#define MY_MODULE_NAME libOriginalModelSample
#include <ASRCAISim1/Factory.h>
#include "R3AgentSample01.h"
#include "R3AgentSample02.h"
#include "R3RewardSample01.h"
#include "WinLoseReward.h"
#include <iostream>
namespace py=pybind11;


PYBIND11_MODULE(MY_MODULE_NAME,m)
{    
    using namespace pybind11::literals;
    m.doc()="OriginalModelSample";
    exportR3AgentSample01(m);
    exportR3AgentSample02(m);
    exportR3RewardSample01(m);
    exportWinLoseReward(m);
    FACTORY_ADD_CLASS(Agent,R3AgentSample01)
    FACTORY_ADD_CLASS(Agent,R3AgentSample02)
    FACTORY_ADD_CLASS(Reward,R3RewardSample01)
    FACTORY_ADD_CLASS(Reward,WinLoseReward)
}
