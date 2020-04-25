#pragma once

#include <iostream>
#include "iterate_struct.h"

#include <rapidjson/document.h>
#include <list>
#include <boost/assert.hpp>

#include "enum_names/enum_names.h"

namespace iterate_struct {

class json_doc_generator
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        m_nodes.back().AddMember(
                    rapidjson::Value(name, m_doc.GetAllocator()),
                    generate_priv(value),
                    m_doc.GetAllocator());
    }

    template<class T>
    static rapidjson::Document generate(const T& value)
    {
        json_doc_generator generator;
        rapidjson::Value val = generator.generate_priv(value);
        generator.m_doc.Swap(val);
        return std::move(generator.m_doc);
    }

private:
    template <
            class T,
            std::enable_if_t<
                std::is_integral_v<T> ||
                std::is_floating_point_v<T>, int> = 0>
    rapidjson::Value generate_priv(const T& x) const
    {
        rapidjson::Value result;
        result.Set(x, m_doc.GetAllocator());
        return result;
    }

    template <
            class T,
            std::enable_if_t<std::is_enum_v<T>, int> = 0>
    rapidjson::Value generate_priv(const T& x) const
    {
        rapidjson::Value result;
        result.SetString(enum_item_name(x), m_doc.GetAllocator());
        return result;
    }

    rapidjson::Value generate_priv(const std::string& x) const
    {
        rapidjson::Value result;
        result.SetString(x.c_str(), m_doc.GetAllocator());
        return result;
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    rapidjson::Value generate_priv(const T& x) const
    {
        m_nodes.emplace_back(rapidjson::kObjectType);
        for_each(x, *this);
        auto result = std::move(m_nodes.back());
        m_nodes.pop_back();
        return result;
    }

    template<class T>
    rapidjson::Value generate_priv(const std::vector<T>& x) const
    {
        rapidjson::Value result(rapidjson::kArrayType);
        for (std::size_t i=0, n=x.size(); i<n; ++i)
            result.PushBack(std::move(generate_priv(x[i])), m_doc.GetAllocator());
        return result;
    }

    mutable rapidjson::Document m_doc;
    mutable std::list<rapidjson::Value> m_nodes;
};

template<class T>
inline rapidjson::Document to_json_doc(const T& value) {
    return json_doc_generator::generate(value);
}



class json_doc_parser
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        auto& node = *m_nodes.back();
        if (!node.IsObject())
            return;
        auto it = node.FindMember(name);
        if(it != node.MemberEnd())
            value = parse_priv<T>(it->value);
    }

    template<class T>
    static T parse(const rapidjson::Document& document) {
        return json_doc_parser().parse_priv<T>(document);
    }

private:

    template <
            class T,
            std::enable_if_t<
                std::is_same_v<T, bool>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsBool())
            throw std::runtime_error("Value in JSON document has an invalid type, expected a boolean");
        return node.GetBool();
    }

    template <
            class T,
            std::enable_if_t<
                !std::is_same_v<T, bool> && std::is_integral_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsInt())
            throw std::runtime_error("Value in JSON document has an invalid type, expected an integer");
        return node.GetInt();
    }

    template <
            class T,
            std::enable_if_t<
                std::is_floating_point_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsNumber())
            throw std::runtime_error("Value in JSON document has an invalid type, expected a number");
        return node.GetDouble();
    }

    template <
            class T,
            std::enable_if_t<
                std::is_enum_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsString())
            throw std::runtime_error("Value in JSON document has an invalid type, expected a string identifying an enumeration value");
        return enum_item_value<T>(node.GetString());
    }

    template <
            class T,
            std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsString())
            throw std::runtime_error("Value in JSON document has an invalid type, expected a string");
        return node.GetString();
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        T result;
        m_nodes.push_back(&node);
        if (!node.IsNull() && node.IsObject())
            for_each(result, *this);
        m_nodes.pop_back();
        return result;
    }

    template <class T, std::enable_if_t<is_vector_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        auto size = node.Size();
        T result;
        result.reserve(size);
        if (!node.IsArray())
            throw std::runtime_error("Value in JSON document has an invalid type, expected an array");
        auto array = node.GetArray();
        for (auto& val : array) {
            result.emplace_back(parse_priv<typename T::value_type>(val));
        }
        return result;
    }

    mutable std::vector<const rapidjson::Value*> m_nodes;
};

template<class T>
inline T from_json_doc(const rapidjson::Document& document) {
    return json_doc_parser::parse<T>(document);
}

} // namespace iterate_struct
