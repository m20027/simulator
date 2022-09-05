#include "CommunicationBuffer.h"
#include "Utility.h"
#include "Units.h"
#include "PhysicalAsset.h"
#include "Controller.h"
#include "Agent.h"
#include "SimulationManager.h"
using namespace util;
CommunicationBuffer::CommunicationBuffer(std::shared_ptr<SimulationManager> manager_,const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_)
:updatedTimes(),manager(manager_),name(name_),buffer(nl::json::object()),
j_participants(participants_),j_inviteOnRequest(inviteOnRequest_){
    if(j_inviteOnRequest.is_string()){
        inviteOnRequest.emplace_back(j_inviteOnRequest);
    }else if(j_inviteOnRequest.is_array()){
        for(auto&& e:j_inviteOnRequest){
            inviteOnRequest.emplace_back(e);
        }
    }
}
CommunicationBuffer::~CommunicationBuffer(){
}
std::string CommunicationBuffer::getName() const{
    return name;
}
void CommunicationBuffer::send(const nl::json& data,CommunicationBuffer::UpdateType updateType){
    assert(data.is_object());
    if(updateType==UpdateType::REPLACE){
        buffer=data;
        for(auto&& e:data.items()){
            updatedTimes[e.key()]=manager.lock()->getTime();
        }
    }else if(updateType==UpdateType::MERGE){
        buffer.merge_patch(data);
        for(auto&& e:data.items()){
            updatedTimes[e.key()]=manager.lock()->getTime();
        }
    }else{
        throw std::runtime_error("Unknown UpdateType");
    }
}
std::pair<double,nl::json> CommunicationBuffer::receive(const std::string& key) const{
    if(updatedTimes.count(key)>0){
        if(buffer.contains(key)){
            return std::make_pair(updatedTimes.at(key),buffer[key]);
        }else{
            return std::make_pair(updatedTimes.at(key),nl::json());
        }
    }else{
        return std::make_pair(-1,nl::json());
    }
}
bool CommunicationBuffer::requestInvitation(std::shared_ptr<Asset> asset){
    for(auto&& query:inviteOnRequest){
        std::regex re;
        bool typeMatch;
        if(query.find("PhysicalAsset:")==0){
            re=std::regex(query.substr(14));
            typeMatch=util::isinstance<PhysicalAsset>(asset);
        }else if(query.find("Controller:")==0){
            re=std::regex(query.substr(11));
            typeMatch=util::isinstance<Controller>(asset);
        }else if(query.find("Agent:")==0){
            re=std::regex(query.substr(6));
            typeMatch=util::isinstance<Agent>(asset);
        }else{
            throw std::runtime_error("Invalid query for CommunicationBuffer's inviteOnRequest. query="+query);
        }
        if(typeMatch && std::regex_match(asset->getFullName(),re)){
            invitationRequested.emplace_back(asset);
            return true;
        }
    }
    return false;
}
void CommunicationBuffer::validate(){
    participants.clear();
    if(j_participants.is_string()){
        validateSub(j_participants);
    }else if(j_participants.is_array()){
        for(auto&& e:j_participants){
            validateSub(e);
        }
    }
    for(auto&& e:invitationRequested){
        participants[e.lock()->getFullName()]=e;
    }
    for(auto&& e:participants){
        e.second.lock()->addCommunicationBuffer(this->shared_from_this());
    }
}
void CommunicationBuffer::validateSub(const std::string& query){
    if(query.find("PhysicalAsset:")==0){
        std::regex re(query.substr(14));
        for(auto&& e:manager.lock()->getAssets([&re](std::shared_ptr<const PhysicalAsset> asset)->bool{
            return std::regex_match(asset->getFullName(),re);
        })){
            participants[e.lock()->getFullName()]=e;
        }
    }else if(query.find("Controller:")==0){
        std::regex re(query.substr(11));
        for(auto&& e:manager.lock()->getControllers([&re](std::shared_ptr<const Controller> cont)->bool{
            return std::regex_match(cont->getFullName(),re);
        })){
            participants[e.lock()->getFullName()]=e;
        }
    }else if(query.find("Agent:")==0){
        std::regex re(query.substr(6));
        for(auto&& e:manager.lock()->getAgents([&re](std::shared_ptr<const Agent> agent)->bool{
            return std::regex_match(agent->getFullName(),re);
        })){
            participants[e.lock()->getFullName()]=e;
        }
    }else{
        throw std::runtime_error("Invalid query for CommunicationBuffer's participants. name="+name+", query="+query);
    }
}
nl::json CommunicationBuffer::to_json_ref(){
    return this->shared_from_this();
}
std::shared_ptr<CommunicationBuffer> CommunicationBuffer::create(std::shared_ptr<SimulationManager> manager_,const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_){
    return std::make_shared<CommunicationBuffer>(manager_,name_,participants_,inviteOnRequest_);
}
std::shared_ptr<CommunicationBuffer> CommunicationBuffer::from_json_ref(const nl::json& j){
    return j;
}

void exportCommunicationBuffer(py::module& m)
{
    using namespace pybind11::literals;
    py::class_<CommunicationBuffer,std::shared_ptr<CommunicationBuffer>> buf(m,"CommunicationBuffer");
    buf
    DEF_FUNC(CommunicationBuffer,send)
    .def("send",[](CommunicationBuffer& v,const py::object& data,CommunicationBuffer::UpdateType updateType){
        v.send(data,updateType);
    })
    DEF_FUNC(CommunicationBuffer,receive)
    ;
    py::enum_<CommunicationBuffer::UpdateType>(buf,"UpdateType")
    .value("MERGE",CommunicationBuffer::UpdateType::MERGE)
    .value("REPLACE",CommunicationBuffer::UpdateType::REPLACE)
    ;
}