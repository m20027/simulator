#include "Utility.h"
namespace util{
    nl::json getValueFromJson(const nl::json& j){
        std::mt19937 gen;gen=std::mt19937(std::random_device()());
        return getValueFromJsonR(j,gen);
    }
    nl::json getValueFromJsonK(const nl::json &j,const std::string& key){
        std::mt19937 gen;gen=std::mt19937(std::random_device()());
        return getValueFromJsonR(j.at(key),gen);
    }

    nl::json merge_patch(const nl::json& base,const nl::json& patch){
        nl::json ret=base;
        ret.merge_patch(patch);
        return ret;
    }
}
void exportUtility(py::module &m){
    py::class_<std::mt19937>(m,"std_mt19937")
    ;
    m.def("getValueFromJson",[](const py::object& j)->py::object{
        return util::getValueFromJson(j);
    });
    m.def("getValueFromJsonKD",[](const py::object &j,const std::string& key,const py::object& defaultValue)->py::object{
        return util::getValueFromJsonKD<nl::json,false>(j,key,defaultValue);
    });
    m.def("getValueFromJsonKRD",[](const py::object &j,const std::string& key,std::mt19937& gen,const py::object& defaultValue)->py::object{
        return util::getValueFromJsonKRD<nl::json,std::mt19937,false>(j,key,gen,defaultValue);
    });
    m.def("getValueFromJsonK",[](const py::object &j,const std::string& key)->py::object{
        return util::getValueFromJsonK(j,key);
    });
    m.def("getValueFromJsonKR",[](const py::object &j,const std::string& key,std::mt19937& gen)->py::object{
        return util::getValueFromJsonKR<std::mt19937>(j,key,gen);
    });
    m.def("merge_patch",&util::merge_patch);
}