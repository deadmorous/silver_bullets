#pragma once

#include "iterate_struct.h"

#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

namespace iterate_struct {

class struct_paths_collector
{
public:
    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        m_current_path_items.push_back(name);
        collect_priv(value);
        m_current_path_items.pop_back();
    }

    template<class T>
    static std::vector<std::string> collect(const T& value)
    {
        struct_paths_collector collector;
        collector.collect_priv(value);
        return collector.m_paths;
    }

private:
    template <
            class T,
            std::enable_if_t<
                std::is_integral_v<T> ||
                std::is_enum_v<T> ||
                std::is_floating_point_v<T> ||
                std::is_same_v<T, std::string>, int> = 0>
    static constexpr bool is_leaf(const T&) {
        return true;
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    static constexpr bool is_leaf(const T&) {
        return false;
    }

    template<class T>
    static constexpr bool is_leaf(const std::vector<T>&)
    {
        return is_leaf(T());
    }

    template <
            class T,
            std::enable_if_t<
                std::is_integral_v<T> ||
                std::is_enum_v<T> ||
                std::is_floating_point_v<T> ||
                std::is_same_v<T, std::string>, int> = 0>
    void collect_priv(const T&) const {
        m_paths.push_back(current_path());
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    void collect_priv(const T& x) const {
        for_each(x, *this);
    }

    template<class T>
    void collect_priv(const std::vector<T>& x) const
    {
        for (std::size_t i=0, n=x.size(); i<n; ++i) {
            if (is_leaf(x))
                m_paths.push_back(current_path());
            else {
                m_current_path_items.push_back(boost::lexical_cast<std::string>(i));
                collect_priv(x[i]);
                m_current_path_items.pop_back();
            }
        }
    }

    std::string current_path() const {
        return std::string("/") + boost::join(m_current_path_items, "/");
    }

    mutable std::vector<std::string> m_current_path_items;
    mutable std::vector<std::string> m_paths;
};

template<class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
inline std::vector<std::string> collect_paths(const T& value) {
    return struct_paths_collector::collect(value);
}

} // namespace iterate_struct
