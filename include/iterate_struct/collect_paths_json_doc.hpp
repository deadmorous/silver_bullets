#pragma once

#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

#include <rapidjson/document.h>

namespace iterate_struct {

class json_doc_paths_collector
{
public:

    static std::vector<std::string> collect(const rapidjson::Document& document)
    {
        json_doc_paths_collector collector;
        collector.collect_priv(document);
        return collector.m_paths;
    }

private:
    static bool is_leaf(const rapidjson::Value& v)
    {
        if (v.IsNumber() || v.IsString())
            return true;
        else if (v.IsArray()) {
            auto a = v.GetArray();
            if (a.Size() == 0)
                return true;
            else {
                auto& v = *a.begin();
                return v.IsNumber() || v.IsArray();
            }
        }
        return true;
    }

    void collect_priv(const rapidjson::Value& value) const
    {
        if (value.IsArray()) {
            auto a = value.GetArray();
            auto n = a.Size();
            for (decltype(n) i=0; i<n; ++i) {
                if (is_leaf(value))
                    m_paths.push_back(current_path());
                else {
                    m_current_path_items.push_back(boost::lexical_cast<std::string>(i));
                    collect_priv(a[i]);
                    m_current_path_items.pop_back();
                }
            }
        }
        else if (value.IsObject()) {
            auto o = value.GetObject();
            for (auto it=o.MemberBegin(); it!=o.MemberEnd(); ++it) {
                m_current_path_items.push_back(it->name.GetString());
                collect_priv(it->value);
                m_current_path_items.pop_back();
            }
        }
        else {
            m_paths.push_back(current_path());
        }
    }

    std::string current_path() const {
        return std::string("/") + boost::join(m_current_path_items, "/");
    }

    mutable std::vector<std::string> m_current_path_items;
    mutable std::vector<std::string> m_paths;
};

inline std::vector<std::string> collect_paths(const rapidjson::Document& document) {
    return json_doc_paths_collector::collect(document);
}

} // namespace iterate_struct
