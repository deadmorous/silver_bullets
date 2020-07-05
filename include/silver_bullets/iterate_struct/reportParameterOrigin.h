#pragma once

#include "collect_paths_struct.hpp"
#include "value_printer.hpp"

#include <boost/range/algorithm/copy.hpp>

#include <set>
#include <map>

namespace silver_bullets {
namespace iterate_struct {

template<class T>
void reportParameterOrigin(
        std::ostream& s,
        const T& value,
        const std::map<std::string, std::string>& configParamOrigin)
{
    using namespace std;

    // Create default parameters; collect paths of actual parameters
    std::map<std::string, std::string> paramOrigin;
    for (auto& path : iterate_struct::collect_paths(value, true))
        paramOrigin[path] = "default";

    std::set<std::string> ultimatePaths;
    boost::range::copy(iterate_struct::collect_paths(value, true), std::inserter(ultimatePaths, ultimatePaths.end()));
    for (auto& item : configParamOrigin) {
        if (ultimatePaths.count(item.first) > 0)
            paramOrigin[item.first] = item.second;
    }

    // Log currently used parameters and the origins of their values
    s << "Currently used parameters" << endl << "----" << endl;
    iterate_struct::print(s, value, [&](const std::string& path) {
        if (ultimatePaths.count(path) > 0) {
            auto it = paramOrigin.find(path);
            s << " (origin: "
                << (it == paramOrigin.end()? string("missing"): paramOrigin.at(path))
                << ")";
        }
    });
}

} // namespace iterate_struct
} // namespace silver_bullets
