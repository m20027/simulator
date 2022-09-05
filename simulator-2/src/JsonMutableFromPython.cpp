#include "JsonMutableFromPython.h"
#include <pybind11_json/pybind11_json.hpp>
#include "Utility.h"
#include "Units.h"

JsonIterator::JsonIterator(nl::json::iterator it_):it(it_){
}
JsonIterator::~JsonIterator(){
}
bool JsonIterator::operator==(const JsonIterator& other) const{
    return it==other.it;
}
bool JsonIterator::operator!=(const JsonIterator& other) const{
    return it!=other.it;
}
bool JsonIterator::operator<(const JsonIterator& other) const{
    return it<other.it;
}
bool JsonIterator::operator<=(const JsonIterator& other) const{
    return it<=other.it;
}
bool JsonIterator::operator>(const JsonIterator& other) const{
    return it>other.it;
}
bool JsonIterator::operator>=(const JsonIterator& other) const{
    return it>=other.it;
}
JsonIterator& JsonIterator::operator+=(nl::json::iterator::difference_type i){
    it+=i;
    return *this;
}
JsonIterator& JsonIterator::operator-=(nl::json::iterator::difference_type i){
    it-=i;
    return *this;
}
JsonIterator JsonIterator::operator+(nl::json::iterator::difference_type i) const{
    return JsonIterator(it+i);
}
JsonIterator operator+(nl::json::iterator::difference_type i, const JsonIterator& it){
    return it+i;
}
JsonIterator JsonIterator::operator-(nl::json::iterator::difference_type i) const{
    return JsonIterator(it-i);
}
nl::json::iterator::difference_type JsonIterator::operator-(const JsonIterator& other) const{
    return it-other.it;
}
nl::json& JsonIterator::operator[](nl::json::iterator::difference_type n) const{
    return it[n];
}
std::string JsonIterator::key(){
    return it.key();
}
nl::json& JsonIterator::value(){
    return it.value();
}
JsonReverseIterator::JsonReverseIterator(nl::json::reverse_iterator it_):it(it_){
}
JsonReverseIterator::~JsonReverseIterator(){
}
bool JsonReverseIterator::operator==(const JsonReverseIterator& other) const{
    return it==other.it;
}
bool JsonReverseIterator::operator!=(const JsonReverseIterator& other) const{
    return it!=other.it;
}
bool JsonReverseIterator::operator<(const JsonReverseIterator& other) const{
    return it<other.it;
}
bool JsonReverseIterator::operator<=(const JsonReverseIterator& other) const{
    return it<=other.it;
}
bool JsonReverseIterator::operator>(const JsonReverseIterator& other) const{
    return it>other.it;
}
bool JsonReverseIterator::operator>=(const JsonReverseIterator& other) const{
    return it>=other.it;
}
JsonReverseIterator& JsonReverseIterator::operator+=(nl::json::reverse_iterator::difference_type i){
    it+=i;
    return *this;
}
JsonReverseIterator& JsonReverseIterator::operator-=(nl::json::reverse_iterator::difference_type i){
    it-=i;
    return *this;
}
JsonReverseIterator JsonReverseIterator::operator+(nl::json::reverse_iterator::difference_type i) const{
    return JsonReverseIterator(it+i);
}
JsonReverseIterator operator+(nl::json::reverse_iterator::difference_type i, const JsonReverseIterator& it){
    return it+i;
}
JsonReverseIterator JsonReverseIterator::operator-(nl::json::reverse_iterator::difference_type i) const{
    return JsonReverseIterator(it-i);
}
nl::json::reverse_iterator::difference_type JsonReverseIterator::operator-(const JsonReverseIterator& other) const{
    return it-other.it;
}
nl::json& JsonReverseIterator::operator[](nl::json::reverse_iterator::difference_type n) const{
    return it[n];
}
std::string JsonReverseIterator::key(){
    return it.key();
}
nl::json& JsonReverseIterator::value(){
    return it.value();
}
JsonIterable::JsonIterable(nl::json& j_,JsonIterable::Type type_):j(j_){
    it=j.begin();
    first=true;
    type=type_;
}
JsonIterable::~JsonIterable(){
}
JsonIterable JsonIterable::iter(){
    return JsonIterable(j,type);
}
py::object JsonIterable::next(){
    if(first){
        first=false;
    }else{
        ++it;
    }
    if(it==j.end()){
        throw pybind11::stop_iteration();
    }
    if(type==Type::KEY){
        return py::str(it.key());
    }else if(type==Type::VALUE){
        return py::cast(it.value());
    }else{//type==Type::ITEM
        return py::make_tuple(it.key(),py::cast(it.value()));
    }
}
JsonConstIterable::JsonConstIterable(const nl::json& j_,JsonConstIterable::Type type_):j(j_){
    it=j.begin();
    first=true;
    type=type_;
}
JsonConstIterable::~JsonConstIterable(){
}
JsonConstIterable JsonConstIterable::iter(){
    return JsonConstIterable(j,type);
}
py::object JsonConstIterable::next(){
    if(first){
        first=false;
    }else{
        ++it;
    }
    if(it==j.end()){
        throw pybind11::stop_iteration();
    }
    if(type==Type::KEY){
        return py::str(it.key());
    }else if(type==Type::VALUE){
        return py::cast(it.value());
    }else{//type==Type::ITEM
        return py::make_tuple(it.key(),py::cast(it.value()));
    }
}

void exportJsonMutableFromPython(py::module& m)
{
    using namespace pybind11::literals;
    py::class_<JsonIterator>(m,"JsonIterator")
    .def("__eq__",&JsonIterator::operator==)
    .def("__neq__",&JsonIterator::operator!=)
    .def("__lt__",&JsonIterator::operator<)
    .def("__le__",&JsonIterator::operator<=)
    .def("__gt__",&JsonIterator::operator>)
    .def("__ge__",&JsonIterator::operator>=)
    .def("__iadd__",&JsonIterator::operator+=,py::keep_alive<0,1>())
    .def("__isub__",&JsonIterator::operator-=,py::keep_alive<0,1>())
    .def("__add__",&JsonIterator::operator+,py::keep_alive<0,1>())
    .def("__radd__",py::overload_cast<nl::json::iterator::difference_type,const JsonIterator&>(&operator+),py::keep_alive<0,2>())
    .def("__sub__",py::overload_cast<nl::json::iterator::difference_type>(&JsonIterator::operator-,py::const_),py::keep_alive<0,1>())
    .def("__sub__",py::overload_cast<const JsonIterator&>(&JsonIterator::operator-,py::const_),py::keep_alive<0,1>())
    .def("__getitem__",&JsonIterator::operator[])
    DEF_FUNC(JsonIterator,key)
    DEF_FUNC(JsonIterator,value)
    ;
    py::class_<JsonReverseIterator>(m,"JsonReverseIterator")
    .def("__eq__",&JsonReverseIterator::operator==)
    .def("__neq__",&JsonReverseIterator::operator!=)
    .def("__lt__",&JsonReverseIterator::operator<)
    .def("__le__",&JsonReverseIterator::operator<=)
    .def("__gt__",&JsonReverseIterator::operator>)
    .def("__ge__",&JsonReverseIterator::operator>=)
    .def("__iadd__",&JsonReverseIterator::operator+=,py::keep_alive<0,1>())
    .def("__isub__",&JsonReverseIterator::operator-=,py::keep_alive<0,1>())
    .def("__add__",&JsonReverseIterator::operator+,py::keep_alive<0,1>())
    .def("__radd__",py::overload_cast<nl::json::reverse_iterator::difference_type,const JsonReverseIterator&>(&operator+),py::keep_alive<0,2>())
    .def("__sub__",py::overload_cast<nl::json::reverse_iterator::difference_type>(&JsonReverseIterator::operator-,py::const_),py::keep_alive<0,1>())
    .def("__sub__",py::overload_cast<const JsonReverseIterator&>(&JsonReverseIterator::operator-,py::const_),py::keep_alive<0,1>())
    .def("__getitem__",&JsonReverseIterator::operator[])
    DEF_FUNC(JsonReverseIterator,key)
    DEF_FUNC(JsonReverseIterator,value)
    ;
    py::class_<JsonIterable>(m,"JsonIterable")
    .def("__iter__",&JsonIterable::iter,py::keep_alive<0,1>())
    .def("__next__",&JsonIterable::next)
    ;
    py::class_<JsonConstIterable>(m,"JsonConstIterable")
    .def("__iter__",&JsonConstIterable::iter,py::keep_alive<0,1>())
    .def("__next__",&JsonConstIterable::next)
    ;
    ;
    py::class_<nl::json>(m,"nljson")
    .def(py::init<py::object>())
    .def("dump",[](nl::json& j,
                    const int indent = -1,
                    const char indent_char = ' ',
                    const bool ensure_ascii = false){
        return j.dump(indent,indent_char,ensure_ascii);
    })
    .def("type",[](const nl::json& j){return j.type();})
    .def("type_name",[](const nl::json& j){return j.type_name();})
    .def("is_primitive",[](const nl::json& j){return j.is_primitive();})
    .def("is_structured",[](const nl::json& j){return j.is_structured();})
    .def("is_null",[](const nl::json& j){return j.is_null();})
    .def("is_boolean",[](const nl::json& j){return j.is_boolean();})
    .def("is_number",[](const nl::json& j){return j.is_number();})
    .def("is_number_integer",[](const nl::json& j){return j.is_number_integer();})
    .def("is_number_unsigned",[](const nl::json& j){return j.is_number_unsigned();})
    .def("is_number_float",[](const nl::json& j){return j.is_number_float();})
    .def("is_object",[](const nl::json& j){return j.is_object();})
    .def("is_array",[](const nl::json& j){return j.is_array();})
    .def("is_string",[](const nl::json& j){return j.is_string();})
    .def("is_binary",[](const nl::json& j){return j.is_binary();})
    .def("is_discarded",[](const nl::json& j){return j.is_discarded();})
    .def("get",[](const nl::json& j)->py::object{return j.get<py::object>();})
    .def("__call__",[](const nl::json& j)->py::object{return j.get<py::object>();})
    .def("get_to",[](const nl::json& j,py::object& v)->py::object{
        v=nl::adl_serializer<py::object>::from_json(j);
        return v;
    })
    .def("__bool__",[](const nl::json& j)->py::bool_{
        if(j.is_null()){
            return false;
        }else if(j.is_boolean()){
            return j.get<bool>();
        }else if(j.is_number_integer()){
            return py::bool_(py::int_(j.get<nl::json::number_integer_t>()));
        }else if(j.is_number_unsigned()){
            return py::bool_(py::int_(j.get<nl::json::number_unsigned_t>()));
        }else if(j.is_number_float()){
            return py::bool_(py::float_(j.get<nl::json::number_float_t>()));
        }else if(j.is_string()){
            return py::bool_(py::str(j.get<nl::json::string_t>()));
        }else if(j.is_array()){
            return j.size()>0;
        }else if(j.is_object()){
            return j.size()>0;
        }
        throw std::runtime_error("Could not convert a json to bool. json="+j.dump());
    })
    .def("__int__",[](const nl::json& j)->py::int_{
        if(j.is_boolean()){
            return py::int_(py::bool_(j.get<bool>()));
        }else if(j.is_number_integer()){
            return py::int_(j.get<nl::json::number_integer_t>());
        }else if(j.is_number_unsigned()){
            return py::int_(j.get<nl::json::number_unsigned_t>());
        }else if(j.is_number_float()){
            return py::int_(py::float_(j.get<nl::json::number_float_t>()));
        }else if(j.is_string()){
            return py::int_(py::str(j.get<nl::json::string_t>()));
        }
        throw std::runtime_error("Could not convert a json to int. json="+j.dump());
    })
    .def("__float__",[](const nl::json& j){
        if(j.is_boolean()){
            return py::float_(py::bool_(j.get<bool>()));
        }else if(j.is_number_integer()){
            return py::float_(py::int_(j.get<nl::json::number_integer_t>()));
        }else if(j.is_number_unsigned()){
            return py::float_(py::int_(j.get<nl::json::number_unsigned_t>()));
        }else if(j.is_number_float()){
            return py::float_(j.get<nl::json::number_float_t>());
        }else if(j.is_string()){
            return py::float_(py::str(j.get<nl::json::string_t>()));
        }
        throw std::runtime_error("Could not convert a json to int. json="+j.dump());
    })
    .def("__str__",[](const nl::json& j)->py::str{
        return j.dump();
    })
    .def("at",[](nl::json& j,nl::json::size_type idx){
        return j.at(idx);
    })
    .def("at",[](const nl::json& j,nl::json::size_type idx){
        return j.at(idx);
    })
    .def("at",[](nl::json& j,const typename nl::json::object_t::key_type& key){
        return j.at(key);
    })
    .def("at",[](const nl::json& j,const typename nl::json::object_t::key_type& key){
        return j.at(key);
    })
    .def("at_p",[](nl::json& j,const std::string& key){
        return j.at(nl::json::json_pointer(key));
    })
    .def("at_p",[](const nl::json& j,const std::string& key){
        return j.at(nl::json::json_pointer(key));
    })
    .def("__getitem__",[](nl::json& j,nl::json::size_type idx) -> nl::json&{
        return j.at(idx);
    },py::keep_alive<0,1>(),py::return_value_policy::reference)
    .def("__getitem__",[](nl::json& j,const typename nl::json::object_t::key_type& key) -> nl::json&{
        return j.at(key);
    },py::keep_alive<0,1>(),py::return_value_policy::reference)
    .def("__getitem__",[](const nl::json& j,nl::json::size_type idx) -> nl::json{
        return j.at(idx);
    })
    .def("__getitem__",[](const nl::json& j,const typename nl::json::object_t::key_type& key) -> nl::json{
        return j.at(key);
    })
    .def("__setitem__",[](nl::json& j,nl::json::size_type idx,py::object value){
        j[idx]=value;
    })
    .def("__setitem__",[](nl::json& j,const typename nl::json::object_t::key_type& key,py::object value){
        j[key]=value;
    })
    .def("value",[](const nl::json& j,const typename nl::json::object_t::key_type& key,py::object default_value){
        return j.value(key,default_value);
    })
    .def("front",[](const nl::json& j){
        return j.front();
    })
    .def("back",[](const nl::json& j){
        return j.back();
    })
    .def("erase",[](nl::json& j,JsonIterator pos)->JsonIterator{
        return j.erase(pos.it);
    },py::keep_alive<0,1>())
    .def("erase",[](nl::json& j,JsonIterator first,JsonIterator last)->JsonIterator{
        return j.erase(first.it,last.it);
    })
    .def("erase",[](nl::json& j,const typename nl::json::object_t::key_type& key)->std::size_t{
        return j.erase(key);
    })
    .def("erase",[](nl::json& j,const nl::json::size_type idx){
        j.erase(idx);
    })
    .def("find",[](nl::json& j,const std::string&& key)->JsonIterator{
        return j.find(key);
    },py::keep_alive<0,1>())
    .def("count",[](const nl::json& j,const std::string&& key){
        return j.count(key);
    })
    .def("contains",[](const nl::json& j,const std::string&& key){
        return j.contains(key);
    })
    .def("contains_p",[](const nl::json& j,const std::string&& ptr){
        return j.contains(nl::json::json_pointer(ptr));
    })
    .def("begin",[](nl::json& j)->JsonIterator{
        return j.begin();
    },py::keep_alive<0,1>())
    .def("end",[](nl::json& j)->JsonIterator{
        return j.end();
    },py::keep_alive<0,1>())
    .def("rbegin",[](nl::json& j)->JsonReverseIterator{
        return j.rbegin();
    },py::keep_alive<0,1>())
    .def("rend",[](nl::json& j)->JsonReverseIterator{
        return j.rend();
    },py::keep_alive<0,1>())
    .def("__iter__",[](nl::json& j)->JsonIterable{
        assert(j.is_object() || j.is_array());
        if(j.is_object()){
            return JsonIterable(j,JsonIterable::Type::KEY);
        }else{
            return JsonIterable(j,JsonIterable::Type::VALUE);
        }
    },py::keep_alive<0,1>())
    .def("__iter__",[](const nl::json& j)->JsonConstIterable{
        assert(j.is_object() || j.is_array());
        if(j.is_object()){
            return JsonConstIterable(j,JsonConstIterable::Type::KEY);
        }else{
            return JsonConstIterable(j,JsonConstIterable::Type::VALUE);
        }
    },py::keep_alive<0,1>())
    .def("keys",[](nl::json& j)->JsonIterable{
        assert(j.is_object());
        return JsonIterable(j,JsonIterable::Type::KEY);
    },py::keep_alive<0,1>())
    .def("keys",[](const nl::json& j)->JsonConstIterable{
        assert(j.is_object());
        return JsonConstIterable(j,JsonConstIterable::Type::KEY);
    },py::keep_alive<0,1>())
    .def("values",[](nl::json& j)->JsonIterable{
        assert(j.is_object());
        return JsonIterable(j,JsonIterable::Type::VALUE);
    },py::keep_alive<0,1>())
    .def("values",[](const nl::json& j)->JsonConstIterable{
        assert(j.is_object());
        return JsonConstIterable(j,JsonConstIterable::Type::VALUE);
    },py::keep_alive<0,1>())
    .def("items",[](nl::json& j)->JsonIterable{
        assert(j.is_object());
        return JsonIterable(j,JsonIterable::Type::ITEM);
    },py::keep_alive<0,1>())
    .def("items",[](const nl::json& j)->JsonConstIterable{
        assert(j.is_object());
        return JsonConstIterable(j,JsonConstIterable::Type::ITEM);
    },py::keep_alive<0,1>())
    .def("empty",[](const nl::json& j){
        return j.empty();
    })
    .def("size",[](const nl::json& j){
        return j.size();
    })
    .def("__len__",[](const nl::json& j){
        return j.size();
    })
    .def("max_size",[](const nl::json& j){
        return j.max_size();
    })
    .def("clear",[](nl::json& j){
        return j.clear();
    })
    .def("push_back",[](nl::json& j,py::object val){
        return j.push_back(val);
    })
    .def("append",[](nl::json& j,py::object val){
        return j.push_back(val);
    })
    .def("__iadd__",[](nl::json& j,py::object val){
        return j+=val;
    })
    .def("insert",[](nl::json& j, JsonIterator pos, py::object val)->JsonIterator{
        return j.insert(pos.it,val);
    },py::keep_alive<0,1>())
    .def("insert",[](nl::json& j, JsonIterator pos, nl::json::size_type cnt, py::object val)->JsonIterator{
        return j.insert(pos.it,cnt,val);
    },py::keep_alive<0,1>())
    .def("insert",[](nl::json& j, JsonIterator pos, JsonIterator first, JsonIterator last)->JsonIterator{
        return j.insert(pos.it,first.it,last.it);
    },py::keep_alive<0,1>())
    .def("insert",[](nl::json& j, JsonIterator first, JsonIterator last){
        j.insert(first.it,last.it);
    })
    .def("update",[](nl::json& j,py::object j_){
        j.update(j_);
    })
    .def("update",[](nl::json& j,JsonIterator first, JsonIterator last){
        j.update(first.it,last.it);
    })
    .def("swap",[](nl::json& j,nl::json& other){
        j.swap(other);
    })
    .def("flatten",[](const nl::json& j){
        return j.flatten();
    })
    .def("unflatten",[](const nl::json& j){
        return j.unflatten();
    })
    .def("patch",[](const nl::json& j,const py::object& json_patch){
        return j.patch(json_patch);
    })
    .def_static("diff",[](const py::object& source,const py::object& target,const std::string& path=""){
        return nl::json::diff(source,target,path);
    })
    .def("merge_patch",[](nl::json& j,const py::object& apply_patch){
        j.merge_patch(apply_patch);
    })
    .def(py::pickle(
        [](const nl::json& j){
            return py::str(j.dump());
        },
        [](py::str s){
            return nl::json::parse(s.cast<std::string>());
        }
    ))
    ;
    m.def("swap",[](nl::json& left,nl::json& right){
        swap(left,right);
    });
    m.def("__eq__",[](nl::json& lhs,nl::json& rhs){lhs==rhs;});
    m.def("__eq__",[](py::object& lhs,nl::json& rhs){lhs==rhs;});
    m.def("__eq__",[](nl::json& lhs,py::object& rhs){lhs==rhs;});
    m.def("__neq__",[](nl::json& lhs,nl::json& rhs){lhs!=rhs;});
    m.def("__neq__",[](py::object& lhs,nl::json& rhs){lhs!=rhs;});
    m.def("__neq__",[](nl::json& lhs,py::object& rhs){lhs!=rhs;});
    m.def("__lt__",[](nl::json& lhs,nl::json& rhs){lhs<rhs;});
    m.def("__lt__",[](py::object& lhs,nl::json& rhs){lhs<rhs;});
    m.def("__lt__",[](nl::json& lhs,py::object& rhs){lhs<rhs;});
    m.def("__le__",[](nl::json& lhs,nl::json& rhs){lhs<=rhs;});
    m.def("__le__",[](py::object& lhs,nl::json& rhs){lhs<=rhs;});
    m.def("__le__",[](nl::json& lhs,py::object& rhs){lhs<=rhs;});
    m.def("__gt__",[](nl::json& lhs,nl::json& rhs){lhs>rhs;});
    m.def("__gt__",[](py::object& lhs,nl::json& rhs){lhs>rhs;});
    m.def("__gt__",[](nl::json& lhs,py::object& rhs){lhs>rhs;});
    m.def("__ge__",[](nl::json& lhs,nl::json& rhs){lhs>=rhs;});
    m.def("__ge__",[](py::object& lhs,nl::json& rhs){lhs>=rhs;});
    m.def("__ge__",[](nl::json& lhs,py::object& rhs){lhs>=rhs;});
}