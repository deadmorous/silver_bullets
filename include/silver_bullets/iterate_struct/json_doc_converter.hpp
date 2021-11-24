#pragma once

#include <iostream>
#include "iterate_struct.hpp"
#include "JsonValue_fwd.hpp"
#include "validators.hpp"

#include <rapidjson/document.h>
#include <list>

#include <boost/assert.hpp>

#include "silver_bullets/enum_names.hpp"

namespace silver_bullets {
namespace iterate_struct {

class json_generator
{
public:
    json_generator(rapidjson::Document::AllocatorType& allocator) :
        m_allocator(allocator)
    {
    }

    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        m_nodes.back().AddMember(
                    rapidjson::Value(name, m_allocator),
                    generate_priv(value),
                    m_allocator);
    }

    template<class T>
    static rapidjson::Value generate(const T& value, rapidjson::Document::AllocatorType& allocator)
    {
        return json_generator(allocator).generate_priv(value);
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
        result.Set(x, m_allocator);
        return result;
    }

    template <
            class T,
            std::enable_if_t<std::is_enum_v<T>, int> = 0>
    rapidjson::Value generate_priv(const T& x) const
    {
        rapidjson::Value result;
        result.SetString(enum_item_name(x), m_allocator);
        return result;
    }

    rapidjson::Value generate_priv(const std::string& x) const
    {
        rapidjson::Value result;
        result.SetString(x.c_str(), m_allocator);
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
            result.PushBack(std::move(generate_priv(x[i])), m_allocator);
        return result;
    }

    template<class T1, class T2>
    rapidjson::Value generate_priv(const std::pair<T1, T2>& x) const
    {
        rapidjson::Value result(rapidjson::kArrayType);
        result.PushBack(std::move(generate_priv(x.first)), m_allocator);
        result.PushBack(std::move(generate_priv(x.second)), m_allocator);
        return result;
    }

    template<class T1, class T2>
    rapidjson::Value generate_priv(const std::map<T1, T2>& x) const
    {
        rapidjson::Value result(rapidjson::kObjectType);
        for (auto& item : x)
            result.AddMember(
                        rapidjson::Value(boost::lexical_cast<std::string>(item.first).c_str(), m_allocator),
                        generate_priv(item.second),
                        m_allocator);
        return result;
    }

    rapidjson::Value generate_priv(const JsonValue& x) const
    {
        auto doc = x.toRapidjsonDocument();
        rapidjson::Value result(doc.GetType());
        result.CopyFrom(doc, m_allocator, true);
        return result;
    }

    rapidjson::Document::AllocatorType& m_allocator;
    mutable std::list<rapidjson::Value> m_nodes;
};

template<class T>
inline rapidjson::Value to_json(const T& value, rapidjson::Document::AllocatorType& allocator)
{
    return json_generator::generate(value, allocator);
}

template<class T>
inline rapidjson::Document to_json_doc(const T& value)
{
    rapidjson::Document doc;
    auto val = to_json(value, doc.GetAllocator());
    doc.Swap(val);
    return doc;
}



template<class Validator>
class json_parser
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        auto& node = *m_nodes.back();
        if (!node.IsObject())
            return;
        auto it = node.FindMember(name);
        if(it != node.MemberEnd()) {
            ValidatorNodeGuard g(m_validator, name);
            value = parse_priv_<T>(it->value);
        }
    }

    template<class T>
    static std::tuple<T, Validator> parse(const rapidjson::Value& value, Validator&& validator)
    {
        json_parser<Validator> parser(std::move(validator));
        return { parser.parse_priv_<T>(value), std::move(parser.m_validator) };
    }

private:

    explicit json_parser(Validator&& validator) :
        m_validator(std::move(validator))
    {}

    template<class T>
    T parse_priv_(const rapidjson::Value& node) const
    {
        auto result = parse_priv<T>(node);
        m_validator.validate(result, ChildNameGetter(node));
        return result;
    }

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

    template <class T, std::enable_if_t<is_pair_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        if (!node.IsArray())
            throw std::runtime_error("Value in JSON document has an invalid type, expected an array");
        auto array = node.GetArray();
        if (array.Size() != 2)
            throw std::runtime_error("Value in JSON document is an array of an invalid length, expected length 2");
        return T {
            parse_priv<typename T::first_type>(array[0]),
            parse_priv<typename T::second_type>(array[1])
        };
    }

    template <class T, std::enable_if_t<is_map_v<T>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const
    {
        T result;
        if (!node.IsObject())
            throw std::runtime_error("Value in JSON document has an invalid type, expected an object");
        auto object = node.GetObject();
        for (auto& member : object)
            result[boost::lexical_cast<typename T::key_type>(member.name.GetString())] =
                    parse_priv<typename T::mapped_type>(member.value);
        return result;
    }

    template <class T, std::enable_if_t<std::is_same_v<T, JsonValue>, int> = 0>
    T parse_priv(const rapidjson::Value& node) const {
        return T(node);
    }

    class ChildNameGetter
    {
    public:
        std::vector<std::string> operator()() const
        {
            std::vector<std::string> result;
            if (m_node.IsObject()) {
                auto o = m_node.GetObject();
                for (auto it=o.MemberBegin(); it!=o.MemberEnd(); ++it)
                    result.push_back(it->name.GetString());
            }
            return result;
        }

    private:
        explicit ChildNameGetter(const rapidjson::Value& node) :
            m_node(node)
        {}

        const rapidjson::Value& m_node;
        friend class json_parser<Validator>;
    };

    class ValidatorNodeGuard
    {
    public:
        ValidatorNodeGuard(Validator& validator, const char *name) : m_validator(validator) {
            m_validator.enterNode(name);
        }

        ~ValidatorNodeGuard() {
            m_validator.exitNode();
        }

    private:
        Validator& m_validator;
    };

    mutable std::vector<const rapidjson::Value*> m_nodes;
    mutable Validator m_validator;
};

template<class T, class Validator=DefaultValidator>
inline std::tuple<T, Validator>
validated_from_json(const rapidjson::Value& value,
                    Validator&& validator = Validator{})
{
    return json_parser<Validator>::template parse<T>(value, std::move(validator));
}

template<class T, class Validator=DefaultValidator>
inline std::tuple<T, Validator>
validated_from_json_doc(const rapidjson::Document& document,
                        Validator&& validator = Validator{})
{
    return validated_from_json<T>(document, std::move(validator));
}

template<class T, class Validator=DefaultValidator>
inline T from_json(const rapidjson::Value& value,
                   Validator&& validator = Validator{})
{
    return std::get<0>(validated_from_json<T>(value, std::move(validator)));
}

template<class T, class Validator=DefaultValidator>
inline T from_json_doc(const rapidjson::Document& document,
                       Validator&& validator = Validator{})
{
    return std::get<0>(validated_from_json_doc<T>(document, std::move(validator)));
}

} // namespace iterate_struct
} // namespace silver_bullets
