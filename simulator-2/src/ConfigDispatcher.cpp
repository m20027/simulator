#include "ConfigDispatcher.h"
#include "Utility.h"

namespace py=pybind11;


ConfigDispatcherElement::ConfigDispatcherElement(const nl::json& j){
    if(j.is_object()){
        if(j.contains("type") && isBuiltinType(j.at("type"))){
            if(j.at("type")=="group"){
                if(j.contains("elements")){
                    config=j;
                }else{
                    config={
                        {"type","group"},
                        {"names",nl::json::array()},
                        {"elements",nl::json::array()},
                        {"overrider",nl::json::array()},
                        {"instance",nullptr},
                        {"index",-1}
                    };
                    for(auto& e:j.items()){
                        if(e.key()!="type"){
                            config["names"].push_back(e.key());
                            config["elements"].push_back(e.value());
                        }
                    }
                }
            }else if(j.at("type")=="none"){
                config={{"type","none"}};
            }else{
                config=j;
            }
        }else{
            config={
                {"type","direct"},
                {"value",j}
            };
        }
        if(j.contains("overrider")){
            nl::json overrider=j.at("overrider");
            if(overrider.is_array()){
                config.at("overrider")=overrider;
            }else if(overrider.is_object()){
                config.at("overrider")=nl::json::array({overrider});
            }else{
                config.at("overrider")=nl::json::array({
                {
                    {"type","direct"},
                    {"value",overrider},
                    {"instance",nullptr},
                    {"index",-1}
                }});
            }
        }else{
            config["overrider"]=nl::json::array();
        }
    }else{
        config={
            {"type","direct"},
            {"value",j},
            {"overrider",nl::json::array()},
            {"instance",nullptr},
            {"index",-1}
        };
    }
}
ConfigDispatcherElement::~ConfigDispatcherElement(){}
void ConfigDispatcherElement::clear(){
    instances.clear();
}
bool ConfigDispatcherElement::isBuiltinType(const std::string& type){
    static const std::unordered_map<std::string,int> builtins={
        {"alias",0},
        {"broadcast",0},
        {"choice",0},
        {"concatenate",0},
        {"direct",0},
        {"group",0},
        {"none",0}
    };
    return builtins.count(type)>0;
}
nl::json ConfigDispatcherElement::get(ConfigDispatcher& dispatcher,const std::string& instance,const int& index){
    nl::json ret;
    std::string type=config.at("type");
    if(instance!="" && instances.count(instance)>0){
        ret=instances[instance];
    }else{
        if(type=="alias"){
                assert(dispatcher.aliases.count(config.at("alias"))>0);
                std::shared_ptr<ConfigDispatcherElement> element=dispatcher.aliases.at(config.at("alias"));
                std::string sub_instance="";
                if(config.contains("instance") && config.at("instance").is_string()){
                    sub_instance=config.at("instance");
                }
                int sub_index=-1;
                if(config.contains("index") && config.at("index").is_number_integer()){
                    sub_index=config.at("index");
                }
                ret=element->get(dispatcher,sub_instance,sub_index);
        }else if(type=="broadcast"){
                nl::json element=config.at("element");
                std::size_t number;
                bool dispatchAfterBroadcast=false;
                if(config.contains("dispatchAfterBroadcast")){
                    dispatchAfterBroadcast=config.at("dispatchAfterBroadcast");
                }
                std::vector<std::string> names;
                if(config.contains("names") && config.at("names").is_array()){
                    nl::json j_names=config.at("names");
                    number=j_names.size();
                    for(std::size_t i=0;i<number;++i){
                        names.push_back(j_names.at(i));
                    }
                }else{
                    number=config.at("number");
                    for(std::size_t i=0;i<number;++i){
                        names.push_back("Element"+std::to_string(i+1));
                    }
                }
                ret={
                    {"type","group"},
                    {"names",names},
                    {"elements",nl::json::array()},
                    {"overrider",nl::json::array()},
                    {"instance",nullptr},
                    {"index",-1}
                };
                if(dispatchAfterBroadcast){
                    for(std::size_t i=0;i<number;++i){
                        ret["elements"].push_back(dispatcher.get(element));
                    }
                }else{
                    nl::json dispatched=dispatcher.get(element);
                    for(std::size_t i=0;i<number;++i){
                        ret["elements"].push_back(dispatched);
                    }
                }
        }else if(type=="choice"){
                nl::json weights=config.at("weights");
                nl::json candidates=config.at("candidates");
                std::discrete_distribution<std::size_t> dist(weights.begin(),weights.end());
                ret=dispatcher.get(candidates[dist(dispatcher.randomGen)]);
        }else if(type=="concatenate"){
                nl::json elements=config.at("elements");
                assert(elements.is_array());
                ret={
                    {"type","group"},
                    {"names",nl::json::array()},
                    {"elements",nl::json::array()},
                    {"overrider",nl::json::array()},
                    {"instance",nullptr},
                    {"index",-1}
                };
                std::size_t totalCount=0;
                for(std::size_t i=0;i<elements.size();++i){
                    nl::json sub=dispatcher.get(elements.at(i));
                    if(sub.at("type")=="group"){
                        for(std::size_t j=0;j<sub.at("elements").size();++j){
                            ret["names"].push_back(sub.at("names").at(j));
                            ret["elements"].push_back(sub.at("elements").at(j));
                            totalCount++;
                        }
                    }else{
                        ret["names"].push_back("Element"+std::to_string(totalCount+1));
                        ret["elements"].push_back(sub);
                    }
                }
        }else if(type=="direct"){
                ret=config;
        }else if(type=="group"){
                nl::json elements=config.at("elements");
                assert(elements.is_array());
                std::string order="fixed";
                if(config.contains("order")){
                    if(config.at("order").is_string()){
                        order=config.at("order");
                    }
                }
                std::vector<std::string> names;
                if(config.contains("names") && config.at("names").is_array()){
                    nl::json j_names=config.at("names");
                    for(std::size_t i=0;i<j_names.size();++i){
                        names.push_back(j_names.at(i));
                    }
                }else{
                    for(std::size_t i=0;i<elements.size();++i){
                        names.push_back("Element"+std::to_string(i+1));
                    }
                }
                assert(elements.size()==names.size());
                std::vector<std::size_t> indices(elements.size());
                for(std::size_t i=0;i<elements.size();++i){indices[i]=i;}
                if(order=="shuffled"){
                    std::shuffle(indices.begin(),indices.end(),dispatcher.randomGen);
                }
                ret={
                    {"type","group"},
                    {"names",names},
                    {"elements",nl::json::array()},
                    {"overrider",nl::json::array()},
                    {"instance",nullptr},
                    {"index",-1}
                };
                for(std::size_t i=0;i<elements.size();++i){
                    ret["elements"].push_back(dispatcher.get(elements.at(indices[i])));
                }
        }else if(type=="none"){
            ret={
                {"type","none"}
            };
        }else{
            throw std::runtime_error("unknown type.");
        }
        if(config.at("overrider").size()>0){
            for(auto& e:config.at("overrider")){
                nl::json value=ConfigDispatcherElement(e).get(dispatcher);
                if(ret.at("type")=="group"){
                    assert(value.at("type")=="group" && ret.at("elements").size()==value.at("elements").size());
                    for(std::size_t i=0;i<ret.at("elements").size();++i){
                        if(ret.at("elements").at(i).at("type")!="none"){
                            ret.at("elements").at(i).merge_patch(value.at("elements").at(i));
                        }
                    }
                }else if(ret.at("type")=="none"){
                    //ret={{"type","none"}};
                }else{
                    ret.merge_patch(value);
                }
            }
        }
        if(instance!=""){
            instances[instance]=ret;
        }
    }
    if(index>=0 && ret.at("type")=="group" && index<ret.at("elements").size()){
        return ret.at("elements").at(index);
    }else{
        return ret;
    }
}

ConfigDispatcher::ConfigDispatcher():config(){
    randomGen=std::mt19937(std::random_device()());
}
ConfigDispatcher::ConfigDispatcher(const nl::json& j){
    ConfigDispatcher();
    initialize(j);
}
ConfigDispatcher::~ConfigDispatcher(){}
nl::json ConfigDispatcher::get(const nl::json& query){
    ConfigDispatcherElement element(query);
    return element.get(*this);
}
void ConfigDispatcher::clear(){
    aliases.clear();
}
void ConfigDispatcher::reset(){
    for(auto& e:aliases){
        e.second->clear();
    }
}
void ConfigDispatcher::initialize(const nl::json& j){
    clear();
    config=j;
    assert(config.is_object());
    for(auto& e:config.items()){
        aliases[e.key()]=std::make_shared<ConfigDispatcherElement>(e.value());
    }
}
void ConfigDispatcher::seed(const unsigned int& seed_){
    randomGen.seed(seed_);
}
nl::json ConfigDispatcher::run(const nl::json& query){
    return sanitize(get(query));
}
nl::json ConfigDispatcher::sanitize(const nl::json& src){
    if(src.is_object()){
        if(src.contains("type")){
            if(src.at("type")=="direct"){
                return sanitize(src.at("value"));
            }else if(src.at("type")=="group"){
                nl::json ret=nl::json::object();
                ret["type"]="group";
                for(std::size_t i=0;i<src["names"].size();++i){
                    ret[src["names"][i].get<std::string>()]=sanitize(src["elements"][i]);
                }
                return ret;
            }else if(src.at("type")=="none"){
                return nl::json(nullptr);
            }else{
                return src;
            }
        }else{
            nl::json ret=nl::json::object();
            for(auto& e:src.items()){
                auto tmp=sanitize(e.value());
                if(!tmp.is_null()){
                    ret[e.key()]=tmp;
                }
            }
            return ret;
        }
    }else{
        return src;
    }
}
std::map<std::string,nl::json> RecursiveJsonExtractor::run(const nl::json& root,std::function<bool(const nl::json&)> checker){
    return std::move(sub(root,"/",checker));
}
std::map<std::string,nl::json> RecursiveJsonExtractor::sub(const nl::json& node,const std::string& path,std::function<bool(const nl::json&)> checker){
    std::map<std::string,nl::json> ret;
    if(checker(node)){
        if(path=="/"){
            ret["/"]=node;
        }else{
            ret[path.substr(0,path.size()-1)]=node;
        }
    }else{
        if(node.is_object()){
            for(auto& e:node.items()){
                auto subvec=sub(e.value(),path+e.key()+"/",checker);
                for(auto& s:subvec){
                    ret[s.first]=s.second;
                }
            }
        }else if(node.is_array()){
            int i=0;
            for(auto& e:node){
                auto subvec=sub(e,path+"["+std::to_string(i)+"]/",checker);
                for(auto& s:subvec){
                    ret[s.first]=s.second;
                }
                ++i;
            }
        }else{
            //do nothing
        }
    }
    return std::move(ret);
}