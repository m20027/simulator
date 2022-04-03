#pragma once
#include "Common.h"
#include <iostream>
#include <random>
#include <memory>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>

namespace py=pybind11;
namespace nl=nlohmann;

class ConfigDispatcher;
class PYBIND11_EXPORT ConfigDispatcherElement{
    nl::json config;
    public:
    std::map<std::string,nl::json> instances;
    ConfigDispatcherElement(const nl::json& j);
    ~ConfigDispatcherElement();
    void clear();
    bool isBuiltinType(const std::string& type);
    nl::json get(ConfigDispatcher& dispatcher,const std::string& instance="",const int& index=-1);
};

class PYBIND11_EXPORT ConfigDispatcher{
    friend class ConfigDispatcherElement;
    std::mt19937 randomGen;
    nl::json config;
    nl::json get(const nl::json& query);
    nl::json sanitize(const nl::json& src);
    public:
    std::map<std::string,std::shared_ptr<ConfigDispatcherElement>> aliases;
    ConfigDispatcher();
    ConfigDispatcher(const nl::json& j);
    ~ConfigDispatcher();
    void clear();
    void reset();
    void initialize(const nl::json& j);
    void seed(const unsigned int& seed_);
    nl::json run(const nl::json& query);
};

class PYBIND11_EXPORT RecursiveJsonExtractor{
    public:
    static std::map<std::string,nl::json> run(const nl::json& root,std::function<bool(const nl::json&)> checker);
    static std::map<std::string,nl::json> sub(const nl::json& node,const std::string& path,std::function<bool(const nl::json&)> checker);
};