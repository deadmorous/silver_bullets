#pragma once

#include <iostream>
#include "iterate_struct.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <list>
#include <vector>

namespace iterate_struct {

class ptree_generator
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        m_nodes.back().put_child(
                    name,
                    std::move(generate_priv(value)));    // Maybe one day there will be an overload taking rvalue
    }

    template<class T>
    static boost::property_tree::ptree generate(const T& value) {
        return ptree_generator().generate_priv(value);
    }

private:
    template <
            class T,
            std::enable_if_t<
                std::is_integral_v<T> ||
                std::is_floating_point_v<T> ||
                std::is_same_v<T, std::string>, int> = 0>
    boost::property_tree::ptree generate_priv(const T& x) const
    {
        boost::property_tree::ptree result;
        result.put_value(x);
        return result;
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    boost::property_tree::ptree generate_priv(const T& x) const
    {
        m_nodes.emplace_back();
        for_each(x, *this);
        auto result = m_nodes.back();
        m_nodes.pop_back();
        return result;
    }

    template<class T>
    boost::property_tree::ptree generate_priv(const std::vector<T>& x) const
    {
        boost::property_tree::ptree result;
        for (std::size_t i=0, n=x.size(); i<n; ++i) {
            result.put_child(
                        boost::lexical_cast<std::string>(i),
                        std::move(generate_priv(x[i])));
        }
        return result;
    }

    mutable std::list<boost::property_tree::ptree> m_nodes;
};

template<class T>
inline boost::property_tree::ptree to_ptree(const T& value) {
    return ptree_generator::generate(value);
}



class ptree_parser
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const {
        value = parse_priv<T>(m_nodes.back()->get_child(name));
    }

    template<class T>
    static T parse(const boost::property_tree::ptree& node) {
        return ptree_parser().parse_priv<T>(node);
    }

private:
    template <
            class T,
            std::enable_if_t<
                std::is_integral_v<T> ||
                std::is_floating_point_v<T> ||
                std::is_same_v<T, std::string>, int> = 0>
    T parse_priv(const boost::property_tree::ptree& node) const {
        return node.get_value<T>();
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    T parse_priv(const boost::property_tree::ptree& node) const
    {
        T result;
        m_nodes.push_back(&node);
        for_each(result, *this);
        m_nodes.pop_back();
        return result;
    }

    template <class T, std::enable_if_t<is_vector_v<T>, int> = 0>
    T parse_priv(const boost::property_tree::ptree& node) const
    {
        T result;
        for (std::size_t i=0; ; ++i) {
            auto child = node.get_child_optional(boost::lexical_cast<std::string>(i));
            if (child)
                result.emplace_back(parse_priv<typename T::value_type>(child.get()));
            else break;
        }
        return result;
    }

    mutable std::vector<const boost::property_tree::ptree*> m_nodes;
};

template<class T>
inline T from_ptree(const boost::property_tree::ptree& node) {
    return ptree_parser::parse<T>(node);
}

} // namespace iterate_struct
