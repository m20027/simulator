#pragma once
#include "Common.h"
#include <memory>
#include <random>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include "Utility.h"
#include "JsonMutableFromPython.h"
namespace py=pybind11;
namespace nl=nlohmann;
enum class SimPhase;

DECLARE_BASE_CLASS(Entity)
    public:
    std::string factoryBaseName;//FactoryにおけるbaseNameが格納される。
    std::string factoryClassName;//Factoryにおけるクラス名が格納される。
    std::string factoryModelName;//Factoryにおけるモデル名が格納される。
    const bool isDummy;//ダミーとして生成されたか否か。コンストラクタ引数に両方null(None)を与えた際にTrueとなる。
    std::mt19937 randomGen;
    nl::json modelConfig,instanceConfig;
    std::map<SimPhase,std::uint64_t> firstTick,interval;
    //constructors & destructor
    Entity(const nl::json& modelConfig_,const nl::json& instanceConfig_);//両方にnullを入れた場合、ダミーとして扱うこととし、コンストラクタの処理をバイパスする等してエラー無くインスタンス生成ができるようにすること。
    virtual ~Entity();
    void seed(const unsigned int& seed_);
    std::string getFactoryBaseName() const;//FactoryにおけるbaseNameを返す。
    std::string getFactoryClassName() const;//Factoryにおけるクラス名を返す。
    std::string getFactoryModelName() const;//Factoryにおけるモデル名を返す。
    virtual std::uint64_t getFirstTick(const SimPhase& phase) const;
    virtual std::uint64_t getNextTick(const SimPhase& phase,const std::uint64_t now);
    virtual bool isSame(std::shared_ptr<Entity> other);
    std::shared_ptr<Entity> child;
    std::weak_ptr<Entity> weakChild;
    void setChild(std::shared_ptr<Entity> child);
    void setWeakChild(std::shared_ptr<Entity> child);
    std::shared_ptr<Entity> getChild();
    py::object getWeakChildPY();
    virtual nl::json to_json_ref();
    template<class T=Entity>
    static std::shared_ptr<T> create(const nl::json& modelConfig_,const nl::json& instanceConfig_){
        return std::make_shared<T>(modelConfig_,instanceConfig_);
    }
    template<class T=Entity>
    static std::shared_ptr<T> from_json_ref(const nl::json& j){
        std::shared_ptr<Entity> ret=j;
        return std::dynamic_pointer_cast<T>(ret);
    }
    template<class T=Entity>
    static std::weak_ptr<T> from_json_weakref(const nl::json& j){
        return j;
    }
};
DECLARE_BASE_TRAMPOLINE(Entity)
    virtual nl::json to_json_ref() override{
        PYBIND11_OVERRIDE(nl::json,Base,to_json_ref);
    }
    virtual std::uint64_t getFirstTick(const SimPhase& phase) const override{
        PYBIND11_OVERRIDE(std::uint64_t,Base,getFirstTick,phase);
    }
    virtual std::uint64_t getNextTick(const SimPhase& phase,const std::uint64_t now) override{
        PYBIND11_OVERRIDE(std::uint64_t,Base,getNextTick,phase,now);
    }
    virtual bool isSame(std::shared_ptr<Entity> other) override{
        PYBIND11_OVERRIDE(bool,Base,isSame,other);
    }
};

void exportEntity(py::module &m);
