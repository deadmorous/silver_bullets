#pragma once

#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

#include <rapidjson/document.h>

namespace iterate_struct {

class json_doc_paths_collector
{
public:

    static std::vector<std::string> collect(const rapidjson::Document& document, bool leavesOnly)
    {
        json_doc_paths_collector collector;
        collector.collect_priv(document, leavesOnly);
        return collector.m_paths;
    }

private:
    void collect_priv(const rapidjson::Value& value, bool leavesOnly) const
    {
        if (!leavesOnly)
            m_paths.push_back(current_path());
        if (value.IsArray()) {
            auto a = value.GetArray();
            auto n = a.Size();
            for (decltype(n) i=0; i<n; ++i) {
                m_current_path_items.push_back(boost::lexical_cast<std::string>(i));
                collect_priv(a[i], leavesOnly);
                m_current_path_items.pop_back();
            }
        }
        else if (value.IsObject()) {
            auto o = value.GetObject();
            for (auto it=o.MemberBegin(); it!=o.MemberEnd(); ++it) {
                m_current_path_items.push_back(it->name.GetString());
                collect_priv(it->value, leavesOnly);
                m_current_path_items.pop_back();
            }
        }
        else if (leavesOnly)
            m_paths.push_back(current_path());
    }

    std::string current_path() const {
        return std::string("/") + boost::join(m_current_path_items, "/");
    }

    mutable std::vector<std::string> m_current_path_items;
    mutable std::vector<std::string> m_paths;
};

inline std::vector<std::string> collect_paths(const rapidjson::Document& document, bool leavesOnly) {
    return json_doc_paths_collector::collect(document, leavesOnly);
}

} // namespace iterate_struct
