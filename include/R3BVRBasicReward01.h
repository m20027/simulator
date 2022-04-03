#pragma once
#include <vector>
#include <map>
#include <pybind11/pybind11.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "MathUtility.h"
#include "Utility.h"
#include "Reward.h"
#include "R3BVRRuler01.h"
namespace py=pybind11;
namespace nl=nlohmann;

DECLARE_CLASS_WITH_TRAMPOLINE(R3BVRBasicReward01,TeamReward)
    public:
	/*令和３年度版の空対空目視外戦闘のルール(その1)の得点に対応した基本報酬の実装例。
        R3BVRRuler01では、得点の計算は戦闘終了時点まで保留される。
        報酬を与える際には、得点増減の要因となった事象が発生した時点で直ちに与えたいということも想定される。
        そのため、陣営ごとの即時報酬として計算するクラスのひな形を実装する。
        また、各得点増減要因の発生時の報酬を変化させたい場合は、modelConfigの以下のパラメータを設定することで実現可能。（デフォルトは全て0またはtrue）
        1. rElim：相手が全滅した際の追加報酬
        2. rElimE：自陣営が全滅した際の追加報酬
        3. rBreakRatio：相手の防衛ラインを突破した際の得点を報酬として与える際の倍率(1+rBreakRatio倍)
        4. rBreak：相手の防衛ラインを突破した際の追加報酬
        5. rBreakE：相手に防衛ラインを突破された際の追加報酬
        6. adjustBreakEnd (bool)：防衛ラインの突破で終了した際に進出度に応じた報酬を無効化するかどうか
        7. rTimeup：時間切れとなった際の追加報酬
        8. rDisq：自陣営がペナルティによる敗北条件を満たした際の追加報酬
        9. rDisqE：相手がペナルティによる敗北条件を満たした際の追加報酬
        10. rHitRatio：相手を撃墜した際の得点を報酬として与える際の倍率(1+rHitRatio倍)
        11. rHit：相手を撃墜した際の追加報酬
        12. rHitE：相手に自陣営の機体が撃墜された際の追加報酬
        13. rAdvRatio：進出度合いに応じた得点(の変化率)を報酬として与える際の倍率(1+rAdvRatio倍)
        14. acceptNegativeAdv：相手陣営の方が進出度合いが大きい時も負数として報酬化するかどうか(ゼロサム化を行う場合は意味なし)
        15. rCrashRatio：自陣営の機体が墜落した際の得点を報酬として与える際の倍率(1+rCrashRatio倍)
        16. rCrash：自陣営の機体が墜落した際の追加報酬
        17. rCrashE：相手が墜落した際の追加報酬
        18. rOutRatio：場外ペナルティによる得点を報酬として与える際の倍率(1+rOutRatio倍)
        19. adjustZerosum：相手陣営の得点を減算してゼロサム化するかどうか
	*/
    double rElim,rElimE,rBreakRatio,rBreak,rBreakE,rTimeup,rDisq,rDisqE,rHitRatio,rHit,rHitE,rAdvRatio,rCrashRatio,rCrash,rCrashE,rOutRatio;
    bool adjustBreakEnd,acceptNegativeAdv,adjustZerosum;
    std::shared_ptr<R3BVRRuler01> ruler;
    std::map<std::string,std::string> opponentName;
    std::map<std::string,int> crashCount,hitCount;
    std::map<std::string,double> advPrev;
    std::map<std::string,double> breakTime;
    std::map<std::string,double> disqTime;
    std::map<std::string,double> eliminatedTime;
    //constructors & destructor
    R3BVRBasicReward01(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~R3BVRBasicReward01();
    //functions
    virtual void onCrash(const nl::json& args);
    virtual void onHit(const nl::json& args);
    virtual void onEpisodeBegin();
    virtual void onInnerStepBegin();
    virtual void onInnerStepEnd();
    virtual void onStepEnd();
};
DECLARE_TRAMPOLINE(R3BVRBasicReward01)
    virtual void onCrash(const nl::json& args) override{
        PYBIND11_OVERRIDE(void,Base,onCrash,args);
    }
    virtual void onHit(const nl::json& args) override{
        PYBIND11_OVERRIDE(void,Base,onHit,args);
    }
};

void exportR3BVRBasicReward01(py::module& m);
