#pragma once

#include "iterate_struct.hpp"

#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

namespace iterate_struct {

class struct_paths_collector
{
public:
    explicit struct_paths_collector(bool leavesOnly) :
        m_leavesOnly(leavesOnly)
    {}

    template<class T>
    inline void operator()(T& value, const char *name) const
    {
        m_current_path_items.push_back(name);
        collect_priv(value);
        m_current_path_items.pop_back();
    }

    template<class T>
    static std::vector<std::string> collect(const T& value, bool leavesOnly)
    {
        struct_paths_collector collector(leavesOnly);
        collector.collect_priv(value);
        return collector.m_paths;
    }

private:
    bool m_leavesOnly;

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
        if (!m_leavesOnly)
            m_paths.push_back(current_path());
        for_each(x, *this);
    }

    template<class T>
    void collect_priv(const std::vector<T>& x) const
    {
        if (!m_leavesOnly)
            m_paths.push_back(current_path());
        for (std::size_t i=0, n=x.size(); i<n; ++i) {
            m_current_path_items.push_back(boost::lexical_cast<std::string>(i));
            collect_priv(x[i]);
            m_current_path_items.pop_back();
        }
    }

    template<class T1, class T2>
    void collect_priv(const std::pair<T1, T2>& x) const
    {
        if (!m_leavesOnly)
            m_paths.push_back(current_path());
        m_current_path_items.push_back("0");
        collect_priv(x.first);
        m_current_path_items.pop_back();
        m_current_path_items.push_back("1");
        collect_priv(x.second);
        m_current_path_items.pop_back();
    }

    template<class T1, class T2>
    void collect_priv(const std::map<T1, T2>& x) const
    {
        if (!m_leavesOnly)
            m_paths.push_back(current_path());
        for (auto& item : x) {
            m_current_path_items.push_back(boost::lexical_cast<std::string>(item.first));
            collect_priv(item.second);
            m_current_path_items.pop_back();
        }
    }

    std::string current_path() const {
        return std::string("/") + boost::join(m_current_path_items, "/");
    }

    mutable std::vector<std::string> m_current_path_items;
    mutable std::vector<std::string> m_paths;
};

template<class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
inline std::vector<std::string> collect_paths(const T& value, bool leavesOnly) {
    return struct_paths_collector::collect(value, leavesOnly);
}

} // namespace iterate_struct
