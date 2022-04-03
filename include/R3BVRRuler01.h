#pragma once
#include <vector>
#include <map>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "MathUtility.h"
#include "Utility.h"
#include "Ruler.h"
namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITH_TRAMPOLINE(R3BVRRuler01,Ruler)
    public:
	/*令和３年度版の空対空目視外戦闘のルール(その1)。得点は陣営(team)単位
	1. 座標系はNEDで、地表面をz=0とする。戦域の中心はx=y=0とする。
	2. 戦域の中心から東西(y軸)方向に±dLine[m]の位置に引かれた直線を各々の防衛ラインとする。
	3. 南北(x軸)方向は±dOut[m]までを戦域とする。
    4. 終了条件は以下の通りとし、終了時の得点が高い陣営を勝者とする。
        (1) いずれかの陣営が全滅（被撃墜または墜落）したとき
        (2) いずれかの陣営が相手の防衛ラインを突破したとき
        (3) 制限時間maxTimeが経過したとき
        (4) いずれかの陣営の得点がpDisq以下となったとき
    5. 各ステップの終了時に同時に複数の終了条件が満たされていた場合は、以下の優先度とする。
        高　(1) > (2) > (4) > (3)　低
        ただし、両陣営が同時に同種の終了条件を満たした場合は、終了条件(3)による終了とみなすものとする。
    6. 得点計算は以下の通りとする。
        1. 相手を撃墜した数1機につきpDown点を加算する。(撃墜された側は増減なし)
        2. 終了条件(2)を満たしたとき、突破した陣営にpBreak点を加算する。
        3. 終了条件(1),(3),(4)のいずれかを満たしたとき、両陣営の最前線にいる機体どうしを結んだ線分の中点のy座標の絶対値に応じ、より進出している陣営に1kmあたりpAdv点を加算する。
        4. ペナルティとして、随時以下の減点を与える。
            (a) 墜落(地面に激突)したとき、pDown点を減算する。
            (b) 各内部ステップの終了時に南北方向の場外に出ていたとき、1秒、1kmにつきpOut点を減算する。
	*/
    enum class DownReason{
        CRASH,
        HIT
    };
    enum class EndReason{
        ELIMINATION,
        BREAK,
        TIMEUP,
        PENALTY,
        NOTYET
    };
    double dLine,dOut,hLim;
    std::string westSider,eastSider;
    double pDisq,pBreak,pDown,pAdv,pOut;
    std::map<std::string,int> crashCount,hitCount;
    std::map<std::string,double> leadRange;
    std::map<std::string,double> lastDownPosition;
    std::map<std::string,DownReason> lastDownReason;
    std::map<std::string,double> outDist;
    std::map<std::string,double> eliminatedTime;
    std::map<std::string,double> breakTime;
    std::map<std::string,double> disqTime;
    std::map<std::string,Eigen::Vector2d> forwardAx,sideAx;
    EndReason endReason,endReasonSub;
    //constructors & destructor
    R3BVRRuler01(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~R3BVRRuler01();
    //functions
    virtual void onCrash(const nl::json& args);
    virtual void onHit(const nl::json& args);
    virtual void onEpisodeBegin();
    virtual void onInnerStepBegin();
    virtual void onInnerStepEnd();
    virtual void checkDone();
};
DECLARE_TRAMPOLINE(R3BVRRuler01)
    virtual void onCrash(const nl::json& args) override{
        PYBIND11_OVERRIDE(void,Base,onCrash,args);
    }
    virtual void onHit(const nl::json& args) override{
        PYBIND11_OVERRIDE(void,Base,onHit,args);
    }
};
PYBIND11_MAKE_OPAQUE(std::map<std::string,R3BVRRuler01::DownReason>);

void exportR3BVRRuler01(py::module& m);
