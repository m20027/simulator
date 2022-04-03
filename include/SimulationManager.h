#pragma once
#include "Common.h"
#include <iostream>
#include <memory>
#include <random>
#include <iterator>
#include <vector>
#include <map>
#include <functional>
#include <variant>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include "Utility.h"
#include "Factory.h"
#include "ConfigDispatcher.h"
#include "CommunicationBuffer.h"
namespace py=pybind11;
namespace nl=nlohmann;

class Entity;
class Asset;
class PhysicalAsset;
class PhysicalAssetAccessor;
class Agent;
class Controller;
class Callback;
class Ruler;
class RulerAccessor;
class Reward;
class Viewer;

enum class SimPhase{
    VALIDATE,
    PERCEIVE,
    CONTROL,
    BEHAVE,
    ON_EPISODE_BEGIN,
    ON_STEP_BEGIN,
    ON_INNERSTEP_BEGIN,
    ON_INNERSTEP_END,
    ON_STEP_END,
    ON_EPISODE_END
};
template<class T,class U>
class MapIterable;
template<class T,class U>
class PYBIND11_EXPORT MapIterator:public std::iterator<std::forward_iterator_tag,std::weak_ptr<T>>{
    public:
    typedef const typename std::map<std::string,std::shared_ptr<U>> ContainerType;
    typedef typename ContainerType::const_iterator ItType;
    typedef typename std::shared_ptr<const T> ConstSharedType;
    typedef typename std::weak_ptr<T> WeakType;
    typedef typename std::function<bool(ConstSharedType)> MatcherType;
    MapIterator(ItType it_,MatcherType matcher_,ItType end_):it(it_),matcher(matcher_),end(end_){}
    MapIterator(const MapIterator<T,U>& other):it(other.it),matcher(other.matcher),end(other.end){}
    ~MapIterator(){}
    bool operator==(const MapIterator<T,U>& other){
        return it==other.it;
    }
    bool operator!=(const MapIterator<T,U>& other){
        return it!=other.it;
    }
    const WeakType operator*() const{
        if constexpr (std::is_same_v<T,U>){
            return (*it).second;
        }else{
            return std::dynamic_pointer_cast<T>((*it).second);
        }
    }
    WeakType operator*(){
        if constexpr (std::is_same_v<T,U>){
            return (*it).second;
        }else{
            return std::dynamic_pointer_cast<T>((*it).second);
        }
    }
    const WeakType operator->() const{
        if constexpr (std::is_same_v<T,U>){
            return it->second;
        }else{
            return std::dynamic_pointer_cast<T>(it->second);
        }
    }
    WeakType operator->(){
        if constexpr (std::is_same_v<T,U>){
            return it->second;
        }else{
            return std::dynamic_pointer_cast<T>(it->second);
        }
    }
    MapIterator<T,U>& operator++(){
        while(it!=end){
            ++it;
            if constexpr (std::is_same_v<T,U>){
                if(it==end || matcher(it->second)){
                    return *this;
                }
            }else{
                if(it==end || (util::isinstance<T>(it->second) && matcher(std::dynamic_pointer_cast<T>(it->second)))){
                    return *this;
                }
            }
        }
        return *this;
    }
    MapIterator<T,U> operator++(int){
        while(it!=end){
            ++it;
            if constexpr (std::is_same_v<T,U>){
                if(it==end || matcher(it->second)){
                    return *this;
                }
            }else{
                if(it==end || (util::isinstance<T>(it->second) && matcher(std::dynamic_pointer_cast<T>(it->second)))){
                    return *this;
                }
            }
        }
        return *this;
    }
    private:
    MatcherType matcher;
    ItType it,end;
};
template<class T,class U>
class MapIterable{
    public:
    typedef const typename std::map<std::string,std::shared_ptr<U>> ContainerType;
    typedef MapIterator<T,U> iterator;
    typedef typename std::shared_ptr<const T> ConstSharedType;
    typedef typename std::weak_ptr<T> WeakType;
    typedef typename std::function<bool(ConstSharedType)> MatcherType;
    MapIterable(ContainerType& container_,MatcherType matcher_):container(container_),matcher(matcher_),it(begin()){}
    ~MapIterable(){}
    MapIterable<T,U>& iter(){
        it=begin();
        first=true;
        return *this;
    }
    WeakType next(){
        if(first){
            first=false;
        }else{
            ++it;
        }
        if(it==end()){
            throw pybind11::stop_iteration();
        }
        return *it;
    }
    iterator begin(){
        auto ret=container.begin();
        while(ret!=container.end()){
            if constexpr (std::is_same_v<T,U>){
                if(matcher(ret->second)){
                    return iterator(ret,matcher,container.end());
                }
            }else{
                if(util::isinstance<T>(ret->second) && matcher(std::dynamic_pointer_cast<T>(ret->second))){
                    return iterator(ret,matcher,container.end());
                }
            }
            ++ret;
        }
        return iterator(ret,matcher,container.end());
    }
    iterator end(){
        return iterator(container.end(),matcher,container.end());
    }
    private:
    MatcherType matcher;
    ContainerType& container;
    iterator it;
    bool first=true;
};
template<class T,class U>
class VectorIterable;
template<class T,class U>
class VectorIterator:public std::iterator<std::forward_iterator_tag,std::weak_ptr<T>>{
    public:
    typedef const typename std::vector<std::shared_ptr<U>> ContainerType;
    typedef typename ContainerType::const_iterator ItType;
    typedef typename std::shared_ptr<const T> ConstSharedType;
    typedef typename std::weak_ptr<T> WeakType;
    typedef typename std::function<bool(ConstSharedType)> MatcherType;
    VectorIterator(ItType it_,MatcherType matcher_,ItType end_):it(it_),matcher(matcher_),end(end_){}
    VectorIterator(const VectorIterator<T,U>& other):it(other.it),matcher(other.matcher),end(other.end){}
    ~VectorIterator(){}
    bool operator==(const VectorIterator<T,U>& other){
        return it==other.it;
    }
    bool operator!=(const VectorIterator<T,U>& other){
        return it!=other.it;
    }
    const WeakType operator*() const{
        if constexpr (std::is_same_v<T,U>){
            return *it;
        }else{
            return std::dynamic_pointer_cast<T>(*it);
        }
    }
    const WeakType operator->() const{
        if constexpr (std::is_same_v<T,U>){
            return it->operator();
        }else{
            return std::dynamic_pointer_cast<T>(it->operator());
        }
    }
    VectorIterator<T,U>& operator++(){
        while(it!=end){
            ++it;
            if constexpr (std::is_same_v<T,U>){
                if(it==end || matcher(*it)){
                    return *this;
                }
            }else{
                if(it==end || (util::isinstance<T>(*it) && matcher(std::dynamic_pointer_cast<T>(*it)))){
                    return *this;
                }
            }
        }
        return *this;
    }
    VectorIterator<T,U> operator++(int){
        while(it!=end){
            ++it;
            if constexpr (std::is_same_v<T,U>){
                if(it==end || matcher(*it)){
                    return *this;
                }
            }else{
                if(it==end || (util::isinstance<T>(*it) && matcher(std::dynamic_pointer_cast<T>(*it)))){
                    return *this;
                }
            }
        }
        return *this;
    }
    private:
    MatcherType matcher;
    ItType it,end;
};
template<class T,class U>
class VectorIterable{
    public:
    typedef const typename std::vector<std::shared_ptr<U>> ContainerType;
    typedef VectorIterator<T,U> iterator;
    //typedef const VectorIterator<T,U> const_iterator;
    typedef typename std::shared_ptr<const T> ConstSharedType;
    typedef typename std::weak_ptr<T> WeakType;
    typedef typename std::function<bool(ConstSharedType)> MatcherType;
    VectorIterable(ContainerType& container_,MatcherType matcher_):container(container_),matcher(matcher_),it(begin()){}
    ~VectorIterable(){}
    VectorIterable<T,U>& iter(){
        //std::cout<<"iter()"<<std::endl;
        it=begin();
        first=true;
        return *this;
    }
    WeakType next(){
        //std::cout<<"next()"<<std::endl;
        if(first){
            first=false;
        }else{
            ++it;
        }
        if(it==end()){
            throw pybind11::stop_iteration();
        }
        return *it;
    }
    iterator begin(){
        auto ret=container.begin();
        while(ret!=container.end()){
            if constexpr (std::is_same_v<T,U>){
                if(matcher(*ret)){
                    return iterator(ret,matcher,container.end());
                }
            }else{
                if(util::isinstance<T>(*ret) && matcher(std::dynamic_pointer_cast<T>(*ret))){
                    return iterator(ret,matcher,container.end());
                }
            }
            ++ret;
        }
        return iterator(ret,matcher,container.end());
    }
    iterator end(){
        return iterator(container.end(),matcher,container.end());
    }
    private:
    MatcherType matcher;
    ContainerType& container;
    iterator it;
    bool first=true;
};
struct OrderComparer{
    public:
    typedef std::pair<std::uint64_t,std::size_t> Type;
    bool operator()(const Type& lhs,const Type& rhs) const;
};
class PYBIND11_EXPORT SimulationManager:public std::enable_shared_from_this<SimulationManager>{
    friend class CommunicationBuffer;
    friend class DependencyChecker;
    friend class SimulationManagerAccessorBase;
    friend class SimulationManagerAccessorForCallback;
    friend class SimulationManagerAccessorForPhysicalAsset;
    friend class SimulationManagerAccessorForAgent;
    public:
    typedef SimulationManager PtrBaseType;
    const std::vector<SimPhase> assetPhases,callbackPhases;
    int worker_index,vector_index;
    py::dict observation_space,action_space;
    py::dict get_observation_space();
    py::dict get_action_space();
    void seed(const unsigned int& seed_);
    py::dict reset();
    py::tuple step(const py::dict& action);
    void stopEpisodeExternally(void);
    double getTime() const;
    std::vector<std::string> getTeams() const;
    nl::json getManagerConfig() const;
    nl::json getFactoryModelConfig() const;
    void setViewerType(const std::string& viewerType);
    void requestReconfigure(const nl::json& managerReplacer,const nl::json& factoryReplacer);
    SimulationManager();
    SimulationManager(const nl::json& config_,int worker_index_=0,int vector_index_=0,std::function<nl::json(const nl::json&,int,int)> overrider_=[](const nl::json& j,int w,int v){return j;});
    virtual ~SimulationManager();
    void printOrderedAssets();
    std::map<std::string,bool> dones;
    std::map<std::string,double> scores,rewards,totalRewards;
    std::map<std::string,std::weak_ptr<Agent>> experts;
    bool manualDone;
    private:
    Factory factory;
    nl::json managerConfig;
    std::mt19937 randomGen;
    double baseTimeStep;
    std::uint64_t tickCount,nextAgentTick;
    std::map<SimPhase,std::uint64_t> nextTick;
    std::uint64_t agentInterval;
    std::map<std::string,std::shared_ptr<PhysicalAsset>> assets;
    std::shared_ptr<Viewer> viewer;
    std::shared_ptr<Ruler> ruler;
    std::map<SimPhase,std::map<
        std::pair<std::uint64_t,std::size_t>,
        std::shared_ptr<Asset>,
        OrderComparer
    >> orderedAssets;
    std::map<std::string,std::shared_ptr<Agent>> agents;
    std::map<std::string,std::shared_ptr<Controller>> controllers;
    std::map<std::string,std::shared_ptr<CommunicationBuffer>> communicationBuffers;
    std::map<std::string,std::shared_ptr<Callback>> callbacks,loggers;
    std::map<SimPhase,std::map<
        std::pair<std::uint64_t,std::size_t>,
        std::shared_ptr<Callback>,
        OrderComparer
    >> orderedAllCallbacks1,orderedAllCallbacks2;
    std::vector<std::shared_ptr<Reward>> rewardGenerators;
    std::vector<std::string> teams;
    std::map<std::string,std::vector<std::function<void(const nl::json&)>>> eventHandlers;
    ConfigDispatcher assetConfigDispatcher,agentConfigDispatcher;
    bool reconfigureRequested;
    nl::json reconfigureManagerReplacer,reconfigureFactoryReplacer;
    int numLearners,numExperts,numClones;
    py::dict lastObservations;
    void configure();
    void checkOrder();
    void runAssetPhaseFunc(const SimPhase& phase);
    void runCallbackPhaseFunc(const SimPhase& phase,bool group1);
    void innerStep();
    py::dict makeObs();
    void deployAction(const py::dict& action);
    void gatherDones();
    void gatherRewards();
    void gatherTotalRewards();
    void addEventHandler(const std::string& name,std::function<void(const nl::json&)> handler);
    void triggerEvent(const std::string& name, const nl::json& args);
    std::weak_ptr<Agent> generateAgent(const nl::json& agentConfig,const std::string& agentName,const std::map<std::string,std::shared_ptr<PhysicalAssetAccessor>>& parents);
    std::weak_ptr<Asset> generateAsset(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_);
    std::weak_ptr<Asset> generateAssetByClassName(const std::string& baseName,const std::string& className,const nl::json& modelConfig_,const nl::json& instanceConfig_);
    bool generateCommunicationBuffer(const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_);
    bool requestInvitationToCommunicationBuffer(const std::string& bufferName,std::shared_ptr<Asset> asset);
    void generateAssets();
    void generateCommunicationBuffers();
    public:
    template<class T=PhysicalAsset>
    std::weak_ptr<T> getAsset(const std::string& fullName_) const{
        try{
        if constexpr(std::is_same_v<T,PhysicalAsset>){
            return assets.at(fullName_);
        }else{
            return std::dynamic_pointer_cast<T>(assets.at(fullName_));
        }
        }catch(std::exception& e){
            std::cout<<"getAsset failure."<<std::endl;
            DEBUG_PRINT_EXPRESSION(fullName_)
            throw e;
        }
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets() const{
        return getAssets<T>([](typename MapIterable<T,PhysicalAsset>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets(typename MapIterable<T,PhysicalAsset>::MatcherType matcher) const{
        return MapIterable<T,PhysicalAsset>(assets,matcher);
    }
    template<class T=Agent>
    std::weak_ptr<T> getAgent(const std::string& fullName_) const{
        try{
            std::string pureName=fullName_;
            auto spos=fullName_.find(":");
            if(spos!=std::string::npos){
                pureName=fullName_.substr(0,spos);
            }
            if constexpr(std::is_same_v<T,Agent>){
                return agents.at(pureName);
            }else{
                return std::dynamic_pointer_cast<T>(agents.at(pureName));
            }
        }catch(std::exception& e){
            std::cout<<"getAgent failure."<<std::endl;
            DEBUG_PRINT_EXPRESSION(fullName_)
            throw e;
        }
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents() const{
        return getAgents<T>([](typename MapIterable<T,Agent>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents(typename MapIterable<T,Agent>::MatcherType matcher) const{
        return MapIterable<T,Agent>(agents,matcher);
    }
    template<class T=Controller>
    std::weak_ptr<T> getController(const std::string& fullName_) const{
        try{
        if constexpr(std::is_same_v<T,Controller>){
            return controllers.at(fullName_);
        }else{
            return std::dynamic_pointer_cast<T>(controllers.at(fullName_));
        }
        }catch(std::exception& e){
            std::cout<<"getController failure."<<std::endl;
            DEBUG_PRINT_EXPRESSION(fullName_)
            throw e;
        }
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers() const{
        return getControllers<T>([](typename MapIterable<T,Controller>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers(typename MapIterable<T,Controller>::MatcherType matcher) const{
        return MapIterable<T,Controller>(controllers,matcher);
    }
    template<class T=Ruler>
    std::weak_ptr<T> getRuler() const{
        if constexpr(std::is_same_v<T,Ruler>){
            return ruler;
        }else{
            return std::dynamic_pointer_cast<T>(ruler);
        }
    }
    template<class T=Viewer>
    std::weak_ptr<T> getViewer() const{
        if constexpr(std::is_same_v<T,Viewer>){
            return viewer;
        }else{
            return std::dynamic_pointer_cast<T>(viewer);
        }
    }
    template<class T=Reward>
    VectorIterable<T,Reward> getRewardGenerators() const{
        return getRewardGenerators<T>([](typename VectorIterable<T,Reward>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=Reward>
    VectorIterable<T,Reward> getRewardGenerators(typename VectorIterable<T,Reward>::MatcherType matcher) const{
        return VectorIterable<T,Reward>(rewardGenerators,matcher);
    }
    template<class T=Callback>
    MapIterable<T,Callback> getCallbacks() const{
        return getCallbacks<T>([](typename MapIterable<T,Callback>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=Callback>
    MapIterable<T,Callback> getCallbacks(typename MapIterable<T,Callback>::MatcherType matcher) const{
        return MapIterable<T,Callback>(callbacks,matcher);
    }
    template<class T=Callback>
    MapIterable<T,Callback> getLoggers() const{
        return getLoggers<T>([](typename MapIterable<T,Callback>::ConstSharedType obj)->bool{
            return true;
        });
    }
    template<class T=Callback>
    MapIterable<T,Callback> getLoggers(typename MapIterable<T,Callback>::MatcherType matcher) const{
        return MapIterable<T,Callback>(loggers,matcher);
    }
    virtual nl::json to_json_ref();
    template<class T=SimulationManager>
    static std::shared_ptr<T> create(const nl::json& config_,int worker_index_=0,int vector_index_=0,std::function<nl::json(const nl::json&,int,int)> overrider_=[](const nl::json& j,int w,int v){return j;}){
         std::shared_ptr<T> ret=std::make_shared<T>(config_,worker_index_,vector_index_,overrider_);
        unsigned int seed_;
        try{
            seed_=ret->managerConfig.at("seed");
        }catch(...){
            seed_=std::random_device()();
        }
        ret->seed(seed_);
        ret->configure();
        ret->seed(seed_);
        return ret;
    }
};
template<class Base=SimulationManager>
class SimulationManagerWrap:public Base{
    public:
    using Base::Base;
    virtual nl::json to_json_ref() override{
        PYBIND11_OVERRIDE(nl::json,Base,to_json_ref);
    }
};

class PYBIND11_EXPORT DependencyChecker:public std::enable_shared_from_this<DependencyChecker>{
    private:
    friend class SimulationManager;
    std::weak_ptr<SimulationManager> manager;
    std::map<SimPhase,bool> readiness;
    std::map<SimPhase,std::vector<std::weak_ptr<Asset>>> dependency;
    virtual void clearReadiness();
    virtual bool checkReadiness(const SimPhase &phase);
    virtual void clearDependency();
    public:
    typedef DependencyChecker PtrBaseType;
    DependencyChecker(std::shared_ptr<SimulationManager> manager_);
    virtual ~DependencyChecker();
    virtual void addDependency(const SimPhase& phase,const std::string& fullName_);
    virtual void addDependency(const SimPhase& phase,std::shared_ptr<Asset> asset);
    nl::json to_json_ref();
    static std::shared_ptr<DependencyChecker> create(std::shared_ptr<SimulationManager> manager_);
    static std::shared_ptr<DependencyChecker> create(std::shared_ptr<DependencyChecker> checker_);
    static std::shared_ptr<DependencyChecker> from_json_ref(const nl::json& j);
    static std::weak_ptr<DependencyChecker> from_json_weakref(const nl::json& j);
};
//Accessors
class PYBIND11_EXPORT SimulationManagerAccessorBase:public std::enable_shared_from_this<SimulationManagerAccessorBase>{
    friend class SimulationManager;
    public:
    typedef SimulationManagerAccessorBase PtrBaseType;
    virtual bool expired() const noexcept;
    virtual double getTime() const;
    virtual double getBaseTimeStep() const;
    virtual std::uint64_t getTickCount() const;
    virtual std::uint64_t getAgentInterval() const;
    virtual std::vector<std::string> getTeams() const;
    nl::json to_json_ref();
    static std::shared_ptr<SimulationManagerAccessorBase> from_json_ref(const nl::json& j);
    static std::weak_ptr<SimulationManagerAccessorBase> from_json_weakref(const nl::json& j);
    SimulationManagerAccessorBase(std::shared_ptr<SimulationManager> manager_);
    virtual ~SimulationManagerAccessorBase();
    protected:
    std::weak_ptr<SimulationManager> manager;
    static std::shared_ptr<SimulationManagerAccessorBase> create(std::shared_ptr<SimulationManager> manager_);
    static std::shared_ptr<SimulationManagerAccessorBase> create(std::shared_ptr<SimulationManagerAccessorBase> original_);
};
class PYBIND11_EXPORT SimulationManagerAccessorForCallback :public SimulationManagerAccessorBase{
    friend class SimulationManager;
    public:
    std::shared_ptr<SimulationManagerAccessorForCallback> copy();
    void requestReconfigure(const nl::json& managerReplacer,const nl::json& factoryReplacer);
    void addEventHandler(const std::string& name,std::function<void(const nl::json&)> handler);
    void triggerEvent(const std::string& name, const nl::json& args);
    const std::map<std::string,bool>& dones() const;
    const std::map<std::string,double>& scores() const;
    const std::map<std::string,double>& rewards() const;
    const std::map<std::string,double>& totalRewards() const;
    const std::map<std::string,std::weak_ptr<Agent>>& experts() const;
    int worker_index() const;
    int vector_index() const;
    bool& manualDone();
    void setManualDone(const bool& b);
    template<class T=PhysicalAsset>
    std::weak_ptr<T> getAsset(const std::string& fullName_) const{
        return manager.lock()->getAsset<T>(fullName_);
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets() const{
        return manager.lock()->getAssets<T>();
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets(typename MapIterable<T,PhysicalAsset>::MatcherType matcher) const{
        return manager.lock()->getAssets<T>(matcher);
    }
    template<class T=Agent>
    std::weak_ptr<T> getAgent(const std::string& fullName_) const{
        return manager.lock()->getAgent<T>(fullName_);
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents() const{
        return manager.lock()->getAgents<T>();
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents(typename MapIterable<T,Agent>::MatcherType matcher) const{
        return manager.lock()->getAgents<T>(matcher);
    }
    template<class T=Controller>
    std::weak_ptr<T> getController(const std::string& fullName_) const{
        return manager.lock()->getController<T>(fullName_);
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers() const{
        return manager.lock()->getControllers<T>();
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers(typename MapIterable<T,Controller>::MatcherType matcher) const{
        return manager.lock()->getControllers<T>(matcher);
    }
    template<class T=Ruler>
    std::weak_ptr<T> getRuler() const{
        return manager.lock()->getRuler<T>();
    }
    template<class T=Viewer>
    std::weak_ptr<T> getViewer() const{
        return manager.lock()->getViewer<T>();
    }
    template<class T=Reward>
    VectorIterable<T,Reward> getRewardGenerators() const{
        return manager.lock()->getRewardGenerators<T>();
    }
    template<class T=Reward>
    VectorIterable<T,Reward> getRewardGenerators(typename VectorIterable<T,Reward>::MatcherType matcher) const{
        return manager.lock()->getRewardGenerators<T>(matcher);
    }
    template<class T=Callback>
    MapIterable<T,Callback> getCallbacks() const{
        return manager.lock()->getCallbacks<T>();
    }
    template<class T=Callback>
    MapIterable<T,Callback> getCallbacks(typename MapIterable<T,Callback>::MatcherType matcher) const{
        return manager.lock()->getCallbacks<T>(matcher);
    }
    template<class T=Callback>
    MapIterable<T,Callback> getLoggers() const{
        return manager.lock()->getLoggers<T>();
    }
    template<class T=Callback>
    MapIterable<T,Callback> getLoggers(typename MapIterable<T,Callback>::MatcherType matcher) const{
        return manager.lock()->getLoggers<T>(matcher);
    }
    nl::json getManagerConfig() const;
    nl::json getFactoryModelConfig() const;
    nl::json to_json_ref();
    static std::shared_ptr<SimulationManagerAccessorForCallback> from_json_ref(const nl::json& j);
    static std::weak_ptr<SimulationManagerAccessorForCallback> from_json_weakref(const nl::json& j);
    SimulationManagerAccessorForCallback(std::shared_ptr<SimulationManager> manager_);
    ~SimulationManagerAccessorForCallback();
    private:
    static std::shared_ptr<SimulationManagerAccessorForCallback> create(std::shared_ptr<SimulationManager> manager_);
    static std::shared_ptr<SimulationManagerAccessorForCallback> create(std::shared_ptr<SimulationManagerAccessorForCallback> original_);
};
class PYBIND11_EXPORT SimulationManagerAccessorForPhysicalAsset :public SimulationManagerAccessorBase{
    friend class SimulationManager;
    //Controller also uses this class as an accessor.
    public:
    std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> copy();
    void triggerEvent(const std::string& name, const nl::json& args);
    template<class T=PhysicalAsset>
    std::weak_ptr<T> getAsset(const std::string& fullName_) const{
        return manager.lock()->getAsset<T>(fullName_);
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets() const{
        return manager.lock()->getAssets<T>();
    }
    template<class T=PhysicalAsset>
    MapIterable<T,PhysicalAsset> getAssets(typename MapIterable<T,PhysicalAsset>::MatcherType matcher) const{
        return manager.lock()->getAssets<T>(matcher);
    }
    template<class T=Agent>
    std::weak_ptr<T> getAgent(const std::string& fullName_) const{
        return manager.lock()->getAgent<T>(fullName_);
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents() const{
        return manager.lock()->getAgents<T>();
    }
    template<class T=Agent>
    MapIterable<T,Agent> getAgents(typename MapIterable<T,Agent>::MatcherType matcher) const{
        return manager.lock()->getAgents<T>(matcher);
    }
    template<class T=Controller>
    std::weak_ptr<T> getController(const std::string& fullName_) const{
        return manager.lock()->getController<T>(fullName_);
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers() const{
        return manager.lock()->getControllers<T>();
    }
    template<class T=Controller>
    MapIterable<T,Controller> getControllers(typename MapIterable<T,Controller>::MatcherType matcher) const{
        return manager.lock()->getControllers<T>(matcher);
    }
    template<class T=Ruler>
    std::weak_ptr<T> getRuler() const{
        return manager.lock()->getRuler<T>();
    }
    template<class T=Agent>
    std::weak_ptr<T> generateAgent(const nl::json& agentConfig,const std::string& agentName,const std::map<std::string,std::shared_ptr<PhysicalAssetAccessor>>& parents){
        return util::getShared<T,Agent>(manager.lock()->generateAgent(agentConfig,agentName,parents));
    }
    template<class T=Asset>
    std::weak_ptr<T> generateAsset(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
        return util::getShared<T,Asset>(manager.lock()->generateAsset(baseName,modelName,instanceConfig_));
    }
    template<class T=Asset>
    std::weak_ptr<T> generateAssetByClassName(const std::string& baseName,const std::string& className,const nl::json& modelConfig_,const nl::json& instanceConfig_){
        return util::getShared<T,Asset>(manager.lock()->generateAssetByClassName(baseName,className,modelConfig_,instanceConfig_));
    }
    bool requestInvitationToCommunicationBuffer(const std::string& bufferName,std::shared_ptr<Asset> asset);
    bool generateCommunicationBuffer(const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_);
    template<class T=Asset>
    std::shared_ptr<T> generateUnmanagedChild(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
        //Make sure that generated child must not have children.
        return manager.lock()->factory.create<T>(baseName,modelName,instanceConfig_);
    }        
    template<class T=Asset>
    std::shared_ptr<T> generateUnmanagedChildByClassName(const std::string& baseName,const std::string& className,const nl::json& modelConfig_,const nl::json& instanceConfig_){
        return manager.lock()->factory.createByClassName<T>(baseName,className,modelConfig_,instanceConfig_);
    }
    nl::json to_json_ref();
    static std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> from_json_ref(const nl::json& j);
    static std::weak_ptr<SimulationManagerAccessorForPhysicalAsset> from_json_weakref(const nl::json& j);
    SimulationManagerAccessorForPhysicalAsset(std::shared_ptr<SimulationManager> manager_);
    ~SimulationManagerAccessorForPhysicalAsset();
    private:
    static std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> create(std::shared_ptr<SimulationManager> manager_);
    static std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> create(std::shared_ptr<SimulationManagerAccessorForPhysicalAsset> original_);
};
class PYBIND11_EXPORT SimulationManagerAccessorForAgent :public SimulationManagerAccessorBase{
    friend class SimulationManager;
    public:
    virtual std::shared_ptr<SimulationManagerAccessorForAgent> copy();
    template<class T=Agent,
        std::enable_if_t<std::is_base_of_v<Agent,T>,std::nullptr_t> = nullptr
    >
    std::shared_ptr<T> generateUnmanagedChild(const std::string& baseName,const std::string& modelName,const nl::json& instanceConfig_){
        //Make sure that generated child must not have children.
        return manager.lock()->factory.create<T>(baseName,modelName,instanceConfig_);
    }
    virtual bool requestInvitationToCommunicationBuffer(const std::string& bufferName,std::shared_ptr<Asset> asset);
    virtual std::shared_ptr<RulerAccessor> getRuler() const;
    nl::json to_json_ref();
    static std::shared_ptr<SimulationManagerAccessorForAgent> from_json_ref(const nl::json& j);
    static std::weak_ptr<SimulationManagerAccessorForAgent> from_json_weakref(const nl::json& j);
    SimulationManagerAccessorForAgent(std::shared_ptr<SimulationManager> manager_);
    virtual ~SimulationManagerAccessorForAgent();
    private:
    static std::shared_ptr<SimulationManagerAccessorForAgent> create(std::shared_ptr<SimulationManager> manager_);
    static std::shared_ptr<SimulationManagerAccessorForAgent> create(std::shared_ptr<SimulationManagerAccessorForAgent> original_);
};

void exportSimulationManager(py::module &m);
