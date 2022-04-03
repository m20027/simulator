#pragma once
#include "Common.h"
#include <memory>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
namespace py=pybind11;
namespace nl=nlohmann;

//C++側で保持しているnl::jsonをPython側から変更可能な形でアクセス可能にする。
//get()または__call__でpybind11_jsonに準じたプリミティブ型への変換に対応する。
class PYBIND11_EXPORT JsonIterator{
    public://private:
    nl::json::iterator it;
    public:
    JsonIterator(nl::json::iterator it_);
    ~JsonIterator();
    bool operator==(const JsonIterator& other) const;
    bool operator!=(const JsonIterator& other) const;
    bool operator<(const JsonIterator& other) const;
    bool operator<=(const JsonIterator& other) const;
    bool operator>(const JsonIterator& other) const;
    bool operator>=(const JsonIterator& other) const;
    JsonIterator& operator+=(nl::json::iterator::difference_type i);
    JsonIterator& operator-=(nl::json::iterator::difference_type i);
    JsonIterator operator+(nl::json::iterator::difference_type i) const;
    friend JsonIterator operator+(nl::json::iterator::difference_type i, const JsonIterator& it);
    JsonIterator operator-(nl::json::iterator::difference_type i) const;
    nl::json::iterator::difference_type operator-(const JsonIterator& other) const;
    nl::json& operator[](nl::json::iterator::difference_type n) const;
    std::string key();
    nl::json& value();
};
class PYBIND11_EXPORT JsonReverseIterator{
    friend class JsonMutableFromPython;
    public://private:
    nl::json::reverse_iterator it;
    public:
    JsonReverseIterator(nl::json::reverse_iterator it_);
    ~JsonReverseIterator();
    bool operator==(const JsonReverseIterator& other) const;
    bool operator!=(const JsonReverseIterator& other) const;
    bool operator<(const JsonReverseIterator& other) const;
    bool operator<=(const JsonReverseIterator& other) const;
    bool operator>(const JsonReverseIterator& other) const;
    bool operator>=(const JsonReverseIterator& other) const;
    JsonReverseIterator& operator+=(nl::json::reverse_iterator::difference_type i);
    JsonReverseIterator& operator-=(nl::json::reverse_iterator::difference_type i);
    JsonReverseIterator operator+(nl::json::reverse_iterator::difference_type i) const;
    friend JsonReverseIterator operator+(nl::json::reverse_iterator::difference_type i, const JsonReverseIterator& it);
    JsonReverseIterator operator-(nl::json::reverse_iterator::difference_type i) const;
    nl::json::reverse_iterator::difference_type operator-(const JsonReverseIterator& other) const;
    nl::json& operator[](nl::json::reverse_iterator::difference_type n) const;
    std::string key();
    nl::json& value();
};
class PYBIND11_EXPORT JsonIterable{
    public:
    enum Type{
        KEY,
        VALUE,
        ITEM
    };
    JsonIterable(nl::json& j_,Type type_);
    ~JsonIterable();
    JsonIterable iter();
    py::object next();
    private:
    nl::json& j;
    bool first;
    nl::json::iterator it;
    Type type;
};
class PYBIND11_EXPORT JsonConstIterable{
    public:
    enum Type{
        KEY,
        VALUE,
        ITEM
    };
    JsonConstIterable(const nl::json& j_,Type type_);
    ~JsonConstIterable();
    JsonConstIterable iter();
    py::object next();
    private:
    const nl::json& j;
    bool first;
    nl::json::const_iterator it;
    Type type;
};

void exportJsonMutableFromPython(py::module& m);