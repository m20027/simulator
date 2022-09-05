#pragma once
#include "Common.h"
#include <memory>
#include <regex>
#include <vector>
#include <map>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include "Utility.h"
namespace py=pybind11;
namespace nl=nlohmann;
class SimulationManager;
class Asset;

DECLARE_BASE_CLASS(CommunicationBuffer)
    private:
    friend class SimulationManager;
    std::string name;
    nl::json j_participants,j_inviteOnRequest;
    bool acceptAdditionalParticipation;
    std::map<std::string,std::weak_ptr<Asset>> participants;
    std::vector<std::string> inviteOnRequest;
    std::vector<std::weak_ptr<Asset>> invitationRequested;
    std::map<std::string,double> updatedTimes;
    nl::json buffer;
    std::weak_ptr<SimulationManager> manager;
    //constructors & destructor
    bool requestInvitation(std::shared_ptr<Asset> asset);
    void validate();
    void validateSub(const std::string& query);
    nl::json to_json_ref();
    static std::shared_ptr<CommunicationBuffer> from_json_ref(const nl::json& j);
    static std::weak_ptr<CommunicationBuffer> from_json_weakref(const nl::json& j);
    public:
    enum UpdateType{
        REPLACE=0,
        MERGE=1
    };
    //functions
    CommunicationBuffer(std::shared_ptr<SimulationManager> manager_,const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_);
    virtual ~CommunicationBuffer();
    std::string getName() const;
    void send(const nl::json& data,UpdateType updateType);
    std::pair<double,nl::json> receive(const std::string& key) const;
    static std::shared_ptr<CommunicationBuffer> create(std::shared_ptr<SimulationManager> manager_,const std::string& name_,const nl::json& participants_,const nl::json& inviteOnRequest_);
};

void exportCommunicationBuffer(py::module& m);