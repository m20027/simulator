#pragma once
#include "Common.h"
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/CXX11/Tensor>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <random>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
namespace py=pybind11;
namespace nl=nlohmann;

namespace util{
    template<class Derived,class Base>
    bool isinstance(std::shared_ptr<Base> f){
        std::shared_ptr<Derived> t=std::dynamic_pointer_cast<Derived>(f);
        return (bool)t;
    }
    template<class Derived,class Base>
    bool isinstance(std::shared_ptr<const Base> f){
        std::shared_ptr<const Derived> t=std::dynamic_pointer_cast<const Derived>(f);
        return (bool)t;
    }
    template<class Derived,class Base>
    bool isinstance(std::weak_ptr<Base> f){
        std::shared_ptr<Derived> t=std::dynamic_pointer_cast<Derived>(f.lock());
        return (bool)t;
    }
    template<class Derived,class Base>
    bool isinstance(std::weak_ptr<const Base> f){
        std::shared_ptr<const Derived> t=std::dynamic_pointer_cast<const Derived>(f.lock());
        return (bool)t;
    }
    template<class Dst,class Src>
    std::shared_ptr<Dst> getShared(std::shared_ptr<Src> src){
        if constexpr (std::is_same_v<Dst,Src>){
            return src;
        }else{
            return std::dynamic_pointer_cast<Dst>(src);
        }
    }
    template<class Dst,class Src>
    std::shared_ptr<Dst> getShared(std::weak_ptr<Src> src){
        if constexpr (std::is_same_v<Dst,Src>){
            return src.lock();
        }else{
            return std::dynamic_pointer_cast<Dst>(src.lock());
        }
    }
    template<class Src>
    std::shared_ptr<Src> getShared(std::shared_ptr<Src> src){
        return src;
    }
    template<class Src>
    std::shared_ptr<Src> getShared(std::weak_ptr<Src> src){
        return src.lock();
    }
    template<class E>
    E jsonToEnum(const nl::json& j){
        std::string s=j;
        auto ret=magic_enum::enum_cast<E>(s);
        return ret.value();
    }
    template<class E>
    nl::json enumToJson(const E& e){
        auto s=magic_enum::enum_name(e);
        return s;
    }
}
namespace pybind11{
    namespace detail{
        template <class T> struct type_caster<std::weak_ptr<T>>
        {
        public:
            PYBIND11_TYPE_CASTER(std::weak_ptr<T>, _("weak_ptr"));
            bool load(handle src, bool)
            {
                try {
                    if(PyWeakref_CheckRef(src.ptr())){
                        std::shared_ptr<T> sh=src().cast<std::shared_ptr<T>>();
                        value=std::weak_ptr<T>(sh);
                        return true;
                    }else if(PyWeakref_CheckProxy(src.ptr())){
                        py::handle h(PyWeakref_GetObject(src.ptr()));
                        std::shared_ptr<T> sh=h.cast<std::shared_ptr<T>>();
                        value=std::weak_ptr<T>(sh);
                        return true;
                    }
                    return false;
                }
                catch (...)
                {
                    return false;
                }
            }

            static handle cast(std::weak_ptr<T> src, return_value_policy /* policy */, handle /* parent */)
            {
                module_ weakref=module_::import("weakref");
                return weakref.attr("proxy")(py::cast(src.lock()).release()).release();
            }
        };
        
        template <typename Scalar_, int NumIndices_, int Options_> struct type_caster<Eigen::Tensor<Scalar_,NumIndices_,Options_>>
        {
        public:
            typedef typename Eigen::Tensor<Scalar_,NumIndices_,Options_> TensorType;
            PYBIND11_TYPE_CASTER(TensorType, _("Eigen::Tensor"));
            bool load(handle src, bool)
            {
                try {
                    if(py::isinstance<py::list>(src)){
                        nl::json tmp=src;
                        value=tmp.get<TensorType>();
                        return true;
                    }else if(py::isinstance<py::buffer>(src)){
                        py::list s=py::cast<py::array>(src).attr("tolist")();
                        nl::json tmp=s;
                        value=tmp.get<TensorType>();
                        return true;
                    }
                    return false;
                }
                catch (...)
                {
                    return false;
                }
            }
            static handle cast(TensorType src, return_value_policy /* policy */, handle /* parent */)
            {
                module_ np=module_::import("numpy");
                nl::json tmp=src;
                object obj=np.attr("array")(tmp.get<py::object>());
                return obj.release();
            }
        };

        template<>
        struct type_caster<boost::uuids::uuid>
        {
        public:
            PYBIND11_TYPE_CASTER(boost::uuids::uuid, _("uuid"));
            bool load(handle src, bool)
            {
                try {
                    std::string s=src.attr("__str__")().cast<std::string>();
                    value=boost::uuids::string_generator()(s);
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            static handle cast(boost::uuids::uuid src, return_value_policy /* policy */, handle /* parent */)
            {
                module_ uuid=module_::import("uuid");
                return uuid.attr("UUID")(boost::uuids::to_string(src)).release();
            }
        };
    }
}

namespace nlohmann{
    template<typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
    struct adl_serializer<Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>> {
        typedef typename Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> MatType;
        static void to_json(json& j, const MatType& m) {
            if(m.rows()==1){
                if(m.cols()==1){
                    j=m(0,0);
                }else{
                    for(Eigen::Index c=0;c<m.cols();++c){
                        j.push_back(m(0,c));
                    }
                }
            }else{
                if(m.cols()==1){
                    for(Eigen::Index r=0;r<m.rows();++r){
                        j.push_back(m(r,0));
                    }
                }else{
                    for(Eigen::Index r=0;r<m.rows();++r){
                        json cols=json::array();
                        for(Eigen::Index c=0;c<m.cols();++c){
                            cols.push_back(m(r,c));
                        }
                        j.push_back(cols);
                    }
                }
            }
        }
        static void from_json(const json& j, MatType& m) {
            Eigen::Index j_rows,j_cols;
            if(j.is_array()){
                j_rows=j.size();
                if(0==j_rows){
                    assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==j_rows);
                    assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==0);
                    m.resize(0,0);
                    return;
                }
                json row=j[0];
                if(row.is_array()){
                    j_cols=j[0].size();
                }else if(row.is_number()){
                    j_cols=1;
                }else{
                    std::cout<<j<<std::endl;
                    throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+","+std::to_string(_Cols)+","+std::to_string(_Options)+","+std::to_string(_MaxRows)+","+std::to_string(_MaxCols)+">.");
                }
                assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==j_rows);
                assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==j_cols);
                m.resize(j_rows,j_cols);
                for(Eigen::Index r=0;r<j_rows;++r){
                    row=j[r];
                    if(row.is_array()){
                        assert(row.size()==j_cols);
                        for(Eigen::Index c=0;c<j_cols;++c){
                            m(r,c)=row[c].get<_Scalar>();
                        }
                    }else if(row.is_number()){
                        assert(1==j_cols);
                        m(r,0)=row.get<_Scalar>();
                    }else{
                        std::cout<<j<<std::endl;
                        throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+","+std::to_string(_Cols)+","+std::to_string(_Options)+","+std::to_string(_MaxRows)+","+std::to_string(_MaxCols)+">.");
                    }
                }
            }else if(j.is_number()){
                assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==1);
                assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==1);
                m.resize(1,1);m(0,0)=j.get<_Scalar>();
                return;
            }else{
                std::cout<<j<<std::endl;
                throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+","+std::to_string(_Cols)+","+std::to_string(_Options)+","+std::to_string(_MaxRows)+","+std::to_string(_MaxCols)+">.");
            }
        }
    };
    template<typename _Scalar, int _Rows, int _Options, int _MaxRows>
    struct adl_serializer<Eigen::Matrix<_Scalar, _Rows, 1, _Options, _MaxRows, 1>> {
        typedef typename Eigen::Matrix<_Scalar, _Rows, 1, _Options, _MaxRows, 1> MatType;
        static void to_json(json& j, const MatType& m) {
            if(m.rows()==1){
                j=m(0,0);
            }
            for(Eigen::Index r=0;r<m.rows();++r){
                j.push_back(m(r,0));
            }
        }
        static void from_json(const json& j, MatType& m) {
            Eigen::Index j_rows,j_cols;
            if(j.is_array()){
                j_rows=j.size();
                if(0==j_rows){
                    assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==j_rows);
                    m.resize(0,1);
                    return;
                }
                json row=j[0];
                if(row.is_array()){
                    j_cols=j[0].size();
                }else if(row.is_number()){
                    j_cols=1;
                }else{
                    std::cout<<j<<std::endl;
                    throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+",1,"+std::to_string(_Options)+","+std::to_string(_MaxRows)+",1>.");
                }
                assert(j_rows==1 || j_cols==1);
                if(j_rows==1){
                    assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==j_cols);
                    m.resize(j_cols,1);
                    if(row.is_array()){
                        for(Eigen::Index c=0;c<j_cols;++c){
                            m(c,0)=row[c].get<_Scalar>();
                        }
                    }else if(row.is_number()){
                        m(0,0)=row.get<_Scalar>();
                    }else{
                        std::cout<<j<<std::endl;
                        throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+",1,"+std::to_string(_Options)+","+std::to_string(_MaxRows)+",1>.");
                    }
                }
                if(j_cols==1){
                    assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==j_rows);
                    m.resize(j_rows,1);
                    for(Eigen::Index r=0;r<j_rows;++r){
                        row=j[r];
                        if(row.is_array()){
                            assert(row.size()==1);
                            m(r,0)=row[0].get<_Scalar>();
                        }else if(row.is_number()){
                            m(r,0)=row.get<_Scalar>();
                        }else{
                            std::cout<<j<<std::endl;
                            throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+",1,"+std::to_string(_Options)+","+std::to_string(_MaxRows)+",1>.");
                        }
                    }
                }
            }else if(j.is_number()){
                assert(MatType::RowsAtCompileTime==Eigen::Dynamic || MatType::RowsAtCompileTime==1);
                m.resize(1,1);m(0,0)=j.get<_Scalar>();
                return;
            }else{
                std::cout<<j<<std::endl;
                throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+","+std::to_string(_Rows)+",1,"+std::to_string(_Options)+","+std::to_string(_MaxRows)+",1>.");
            }
        }
    };
    template<typename _Scalar, int _Cols, int _Options, int _MaxCols>
    struct adl_serializer<Eigen::Matrix<_Scalar, 1, _Cols, _Options, 1, _MaxCols>> {
        typedef typename Eigen::Matrix<_Scalar, 1, _Cols, _Options, 1, _MaxCols> MatType;
        static void to_json(json& j, const MatType& m) {
            if(m.cols()==1){
                j=m(0,0);
            }
            for(Eigen::Index c=0;c<m.cols();++c){
                j.push_back(m(0,c));
            }
        }
        static void from_json(const json& j, MatType& m) {
            Eigen::Index j_rows,j_cols;
            if(j.is_array()){
                j_rows=j.size();
                if(0==j_rows){
                    assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==j_rows);
                    m.resize(1,0);
                    return;
                }
                json row=j[0];
                if(row.is_array()){
                    j_cols=j[0].size();
                }else if(row.is_number()){
                    j_cols=1;
                }else{
                    std::cout<<j<<std::endl;
                    throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+",1,"+std::to_string(_Cols)+","+std::to_string(_Options)+",1,"+std::to_string(_MaxCols)+">.");
                }
                assert(j_rows==1 || j_cols==1);
                if(j_rows==1){
                    assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==j_cols);
                    m.resize(1,j_cols);
                    if(row.is_array()){
                        for(Eigen::Index c=0;c<j_cols;++c){
                            m(0,c)=row[c].get<_Scalar>();
                        }
                    }else if(row.is_number()){
                        m(0,0)=row.get<_Scalar>();
                    }else{
                        std::cout<<j<<std::endl;
                        throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+",1,"+std::to_string(_Cols)+","+std::to_string(_Options)+",1,"+std::to_string(_MaxCols)+">.");
                    }
                }
                if(j_cols==1){
                    assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==j_rows);
                    m.resize(1,j_rows);
                    for(Eigen::Index r=0;r<j_rows;++r){
                        row=j[r];
                        if(row.is_array()){
                            assert(row.size()==1);
                            m(0,r)=row[0].get<_Scalar>();
                        }else if(row.is_number()){
                            m(0,r)=row.get<_Scalar>();
                        }else{
                            std::cout<<j<<std::endl;
                            throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+",1,"+std::to_string(_Cols)+","+std::to_string(_Options)+",1,"+std::to_string(_MaxCols)+">.");
                        }
                    }
                }
            }else if(j.is_number()){
                assert(MatType::ColsAtCompileTime==Eigen::Dynamic || MatType::ColsAtCompileTime==1);
                m.resize(1,1);m(0,0)=j.get<_Scalar>();
                return;
            }else{
                std::cout<<j<<std::endl;
                throw std::runtime_error("This json is not convertible to Eigen::Matrix<"+std::string(typeid(_Scalar()).name())+",1,"+std::to_string(_Cols)+","+std::to_string(_Options)+",1,"+std::to_string(_MaxCols)+">.");
            }
        }
    };
    template<typename Scalar_, int NumIndices_, int Options_, typename IndexType_>
    struct adl_serializer<Eigen::Tensor<Scalar_, NumIndices_, Options_, IndexType_>> {
        typedef typename Eigen::Tensor<Scalar_, NumIndices_, Options_, IndexType_> MatType;
        static void to_json(json& j, const MatType& m) {
            struct Dummy{
                nl::json& value;
                Dummy(nl::json& j_):value(j_){}
            };
            Eigen::array<Eigen::Index,NumIndices_> indices;
            for(Eigen::Index i=0;i<NumIndices_;++i){
                indices[i]=0;
            }
            Eigen::Index totalSize=m.size();
            Eigen::array<Eigen::Index,NumIndices_> dimensions=m.dimensions();
            Eigen::Index currentDim=NumIndices_-1;
            std::stack<Dummy*> currentNode;
            nl::json tmp;
            j=nl::json::array();
            Dummy* top;
            currentNode.push(new Dummy(j));
            for(Eigen::Index i=1;i<NumIndices_;++i){
                currentNode.top()->value.push_back(nl::json::array());
                currentNode.push(new Dummy(currentNode.top()->value.at(currentNode.top()->value.size()-1)));
            }
            for(Eigen::Index count=0;count<totalSize;++count){
                currentNode.top()->value.push_back(m(indices));
                indices[currentDim]++;
                while(currentDim>0 && indices[currentDim]==dimensions[currentDim]){
                    indices[currentDim]=0;
                    top=currentNode.top();
                    currentNode.pop();
                    delete top;
                    currentDim--;
                    indices[currentDim]++;
                }
                if(indices[0]<dimensions[0]){
                    for(Eigen::Index i=currentDim;i<NumIndices_-1;++i){
                        currentNode.top()->value.push_back(nl::json::array());
                        currentNode.push(new Dummy(currentNode.top()->value.at(currentNode.top()->value.size()-1)));
                        currentDim++;
                    }
                }
            }
            while(!currentNode.empty()){
                top=currentNode.top();
                currentNode.pop();
                delete top;
            }
        }
        static void from_json(const json& j, MatType& m) {
            struct Dummy{
                const nl::json& value;
                Dummy(const nl::json& j_):value(j_){}
            };
            std::stack<Dummy*> currentNode;
            Dummy* top;
            currentNode.push(new Dummy(j));
            Eigen::array<Eigen::Index,NumIndices_> j_dimensions;
            for(Eigen::Index i=0;i<NumIndices_;++i){
                top=currentNode.top();
                if(top->value.is_array()){
                    j_dimensions[i]=top->value.size();
                    currentNode.push(new Dummy(top->value.at(0)));
                }else if(top->value.is_number()){
                    for(Eigen::Index j=i;j<NumIndices_;++j){
                        j_dimensions[j]=1;
                    }
                    break;
                }else{
                    std::cout<<j<<std::endl;
                    throw std::runtime_error("This json is not convertible to Eigen::Tensor");
                }
            }
            while(!currentNode.empty()){
                top=currentNode.top();
                currentNode.pop();
                delete top;
            }
            try{
                m=MatType(j_dimensions);
                Eigen::array<Eigen::Index,NumIndices_> indices;
                currentNode.push(new Dummy(j));
                indices[0]=0;
                for(Eigen::Index i=1;i<NumIndices_;++i){
                    top=currentNode.top();
                    if(top->value.is_array()){
                        currentNode.push(new Dummy(top->value.at(0)));
                    }else if(top->value.is_number()){
                        currentNode.push(new Dummy(top->value));
                    }else{
                        throw std::runtime_error("This json is not convertible to Eigen::Tensor");
                    }
                    indices[i]=0;
                }
                Eigen::Index totalSize=m.size();
                Eigen::Index currentDim=NumIndices_-1;
                for(Eigen::Index count=0;count<totalSize;++count){
                    top=currentNode.top();
                    if(top->value.is_array()){
                        m(indices)=top->value.at(indices[currentDim]);
                    }else if(top->value.is_number()){
                        m(indices)=top->value;
                    }else{
                        throw std::runtime_error("This json is not convertible to Eigen::Tensor");
                    }
                    indices[currentDim]++;
                    while(currentDim>0 && indices[currentDim]==j_dimensions[currentDim]){
                        indices[currentDim]=0;
                        top=currentNode.top();
                        currentNode.pop();
                        delete top;
                        currentDim--;
                        indices[currentDim]++;
                    }
                    if(indices[0]<j_dimensions[0]){
                        for(Eigen::Index i=currentDim;i<NumIndices_-1;++i){
                            top=currentNode.top();
                            if(top->value.is_array()){
                                currentNode.push(new Dummy(top->value.at(indices[i])));
                            }else if(top->value.is_number()){
                                currentNode.push(new Dummy(top->value));
                            }else{
                                throw std::runtime_error("This json is not convertible to Eigen::Tensor");
                            }
                            currentDim++;
                        }
                    }
                }
                while(!currentNode.empty()){
                    top=currentNode.top();
                    currentNode.pop();
                    delete top;
                }
            }catch(std::exception& e){
                std::cout<<e.what()<<std::endl;
                std::cout<<j<<std::endl;
                throw e;
            }
        }
    };


    template<typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        typedef typename std::shared_ptr<T> PtrType;
        static void to_json(json& j, const PtrType& m) {
            typename T::PtrBaseType* raw=m.get();
            if(raw==nullptr){
                j=0;//not nullptr, because merge_patch automatically deletes nullptr.
            }else{
                j=reinterpret_cast<intptr_t>(raw);
            }
        }
        static void from_json(const json& j, PtrType& m) {
            if(j.get<intptr_t>()==0){//nullptr
                //nothing
            }else{
                typename T::PtrBaseType* raw=reinterpret_cast<T*>(j.get<intptr_t>());
                m=std::dynamic_pointer_cast<T>(raw->shared_from_this());
            }
        }
        private:
        void _dummy(std::enable_shared_from_this<T> *){
            //dummy
        }
    };
    template<typename T>
    struct adl_serializer<std::weak_ptr<T>> {
        typedef typename std::weak_ptr<T> PtrType;
        static void to_json(json& j, const PtrType& m) {
            typename T::PtrBaseType* raw=m.lock().get();
            if(raw==nullptr){
                j=0;//not nullptr, because merge_patch automatically deletes nullptr.
            }else{
                j=reinterpret_cast<intptr_t>(raw);
            }
        }
        static void from_json(const json& j, PtrType& m) {
            if(j.get<intptr_t>()==0){//nullptr
                //nothing
            }else{
                typename T::PtrBaseType* raw=reinterpret_cast<T*>(j.get<intptr_t>());
                m=std::weak_ptr<T>(std::dynamic_pointer_cast<T>(raw->shared_from_this()));
            }
        }
        private:
        void _dummy(std::enable_shared_from_this<T> *){
            //dummy
        }
    };

    template<>
    struct adl_serializer<boost::uuids::uuid> {
        static void to_json(json& j, const boost::uuids::uuid& m) {
            j=boost::uuids::to_string(m);
        }
        static void from_json(const json& j, boost::uuids::uuid& m) {
            assert(j.is_string());
            m=boost::uuids::string_generator()(j.get<std::string>());
        }
    };
}

namespace util{
    template<class URBG>
    nl::json getValueFromJsonR(const nl::json& j, URBG& gen){
        if(j.is_object()){
            std::string type;
            try{
                type=j.at("type");
            }catch(...){
                type="direct";
            }
            if(type=="normal"){
                Eigen::MatrixXd mean=j.at("mean");
                Eigen::MatrixXd stddev=j.at("stddev");
                Eigen::MatrixXd ret(mean.rows(),mean.cols());
                for(Eigen::Index r=0;r<ret.rows();++r){
                    for(Eigen::Index c=0;c<ret.cols();++c){
                        std::normal_distribution<double> dist(mean(r,c),stddev(r,c));
                        ret(r,c)=dist(gen);
                    }
                }
                return ret;
            }else if(type=="uniform"){
                std::string dtype;
                try{
                    dtype=j.at("dtype");
                }catch(...){
                    dtype="float";
                }
                if(dtype=="int"){
                    Eigen::Matrix<int64_t,-1,-1> low=j.at("low");
                    Eigen::Matrix<int64_t,-1,-1> high=j.at("high");
                    Eigen::Matrix<int64_t,-1,-1> ret(low.rows(),low.cols());
                    for(Eigen::Index r=0;r<ret.rows();++r){
                        for(Eigen::Index c=0;c<ret.cols();++c){
                            std::uniform_int_distribution<int64_t> dist(low(r,c),high(r,c));
                            ret(r,c)=dist(gen);
                        }
                    }
                    return ret;
                }else{
                    Eigen::MatrixXd low=j.at("low");
                    Eigen::MatrixXd high=j.at("high");
                    Eigen::MatrixXd ret(low.rows(),low.cols());
                    for(Eigen::Index r=0;r<ret.rows();++r){
                        for(Eigen::Index c=0;c<ret.cols();++c){
                            std::uniform_real_distribution<double> dist(low(r,c),high(r,c));
                            ret(r,c)=dist(gen);
                        }
                    }
                    return ret;
                }
            }else if(type=="choice"){
                nl::json weights=j.at("weights");
                nl::json candidates=j.at("candidates");
                std::discrete_distribution<std::size_t> dist(weights.begin(),weights.end());
                return getValueFromJsonR(candidates[dist(gen)],gen);
            }else{//direct
                if(j.contains("type") && j.contains("value")){
                    return j.at("value");
                }else{
                    nl::json ret=nl::json::object();
                    for(auto& e:j.items()){
                        ret[e.key()]=getValueFromJsonR(e.value(),gen);
                    }

                }
                try{
                    return j.at("value");
                }catch(...){
                    return j;
                }
            }
        }else if(j.is_array()){
            nl::json ret=nl::json::array();
            for(auto& e:j){
                ret.push_back(getValueFromJsonR(e,gen));
            }
            return ret;
        }else{//direct
            return j;
        }
    }
    nl::json PYBIND11_EXPORT getValueFromJson(const nl::json& j);
    template<class URBG>
    nl::json getValueFromJsonKR(const nl::json &j,const std::string& key,URBG& gen){
        return getValueFromJsonR(j.at(key),gen);
    }
    nl::json PYBIND11_EXPORT getValueFromJsonK(const nl::json &j,const std::string& key);
    template<typename ValueType,class URBG,bool ShowExcept=true>
    ValueType getValueFromJsonKRD(const nl::json &j,const std::string& key,URBG& gen,const ValueType& defaultValue){
        if(j.contains(key)){
            return getValueFromJsonR(j.at(key),gen);
        }else{
            return defaultValue;
        }
    }
    template<typename ValueType,bool ShowExcept=true>
    ValueType getValueFromJsonKD(const nl::json &j,const std::string& key,const ValueType& defaultValue){
        std::mt19937 gen;gen=std::mt19937(std::random_device()());
        if(j.contains(key)){
            return getValueFromJsonR(j.at(key),gen);
        }else{
            return defaultValue;
        }
    }
    nl::json PYBIND11_EXPORT merge_patch(const nl::json& base,const nl::json& patch);
}



#define DECLARE_BASE_CLASS(className)\
class className;\
template<class Base>\
class className##Wrap;\
class PYBIND11_EXPORT className:public std::enable_shared_from_this<className>{\
    public:\
    typedef className PtrBaseType;\
    typedef className Type;\
    typedef std::enable_shared_from_this<className> BaseType;\
    template<class T=Type> using TrampolineType=className##Wrap<T>;

#define DECLARE_CLASS_WITH_TRAMPOLINE(className,baseName)\
class className;\
template<class Base>\
class className##Wrap;\
class PYBIND11_EXPORT className:public baseName{\
    public:\
    typedef className Type;\
    typedef baseName BaseType;\
    template<class T=Type> using TrampolineType=className##Wrap<T>;

#define DECLARE_CLASS_WITHOUT_TRAMPOLINE(className,baseName)\
class PYBIND11_EXPORT className:public baseName{\
    public:\
    typedef className Type;\
    typedef baseName BaseType;\
    template<class T=Type> using TrampolineType=BaseType::TrampolineType<T>;

#define DECLARE_BASE_TRAMPOLINE(className)\
template<class Base=className>\
class className##Wrap:public Base{\
    public:\
    using Base::Base;
#define DECLARE_TRAMPOLINE(className)\
template<class Base=className>\
class className##Wrap:public className::BaseType::TrampolineType<Base>{\
    public:\
    template<class T=Base> using BaseTrampoline=className::BaseType::TrampolineType<T>;\
    using BaseTrampoline<Base>::BaseTrampoline;

#define EXPOSE_BASE_CLASS(className)\
py::class_<className,className::TrampolineType<>,std::shared_ptr<className>>(m,#className)\
.def(py::init(&className::create<className::TrampolineType<>>))\
.def(py::init([](const py::object& modelConfig_,const py::object& instanceConfig_){return className::create<className::TrampolineType<>>(modelConfig_,instanceConfig_);}))

#define EXPOSE_CLASS(className)\
py::class_<className,className::BaseType,className::TrampolineType<>,std::shared_ptr<className>>(m,#className)\
.def(py::init(&className::create<className::TrampolineType<>>))\
.def(py::init([](const py::object& modelConfig_,const py::object& instanceConfig_){return className::create<className::TrampolineType<>>(modelConfig_,instanceConfig_);}))

#define EXPOSE_BASE_CLASS_WITHOUT_INIT(className)\
py::class_<className,className::TrampolineType<>,std::shared_ptr<className>>(m,#className)

#define EXPOSE_CLASS_WITHOUT_INIT(className)\
py::class_<className,className::BaseType,className::TrampolineType<>,std::shared_ptr<className>>(m,#className)

#define DEF_FUNC(CLASS,func) .def(#func,&CLASS::func)
#define DEF_STATIC_FUNC(CLASS,func) .def_static(#func,&CLASS::func)
#define DEF_READWRITE(CLASS,field) .def_readwrite(#field,&CLASS::field)
#define DEF_READONLY(CLASS,field) .def_readonly(#field,&CLASS::field)


void exportUtility(py::module &m);

#define DEBUG_PRINT_EXPRESSION(ex) std::cout<<#ex<<"="<<ex<<std::endl;