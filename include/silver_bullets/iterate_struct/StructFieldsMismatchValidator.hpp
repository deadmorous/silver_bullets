#pragma once

#include "iterate_struct.hpp"
#include "ValidatorState.hpp"
#include "TreePathTracker.hpp"

#include <iostream>
#include <set>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>

namespace silver_bullets::iterate_struct {

struct StructFieldsMismatch
{
    std::set<std::string> extraNames;
    std::set<std::string> missingNames;

    operator bool() const {
        return !(extraNames.empty() && missingNames.empty());
    }
};

template<class T, class ChildNameGetter,
         typename = std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>>>
StructFieldsMismatch struct_fields_mismatch(const T& value, const ChildNameGetter& childNameGetter)
{
    auto bkins = [](auto& container) {
        return std::inserter(container, std::end(container));
    };

    std::set<std::string> expectedNames;
    for_each(value, [&](auto&, const char* name) {
        expectedNames.insert(name);
    });

    std::set<std::string> actualNames;
    boost::range::copy(childNameGetter(), bkins(actualNames));

    std::set<std::string> extraNames;
    boost::range::set_difference(actualNames, expectedNames, bkins(extraNames));

    std::set<std::string> missingNames;
    boost::range::set_difference(expectedNames, actualNames, bkins(missingNames));

    return { std::move(extraNames), std::move(missingNames) };
}

class StructFieldsMismatchValidator
{
public:
    template<class T, class ChildNameGetter>
    void validate(ValidatorState& state,
                  const T& value,
                  const TreePathTracker& pathGetter,
                  const ChildNameGetter& childNameGetter)
    {
        if constexpr(has_iterate_struct_helper_v<T>)
            if (auto mismatch = struct_fields_mismatch(value, childNameGetter))
                report( state, mismatch, pathGetter );
    }

    void report(ValidatorState& state,
                const StructFieldsMismatch& mismatch,
                const TreePathTracker& pathGetter)
    {
        auto& s = *state.errorStream;
        s << "At path '" << boost::join(pathGetter.path(), ".") << "':" << std::endl;
        if (!mismatch.extraNames.empty()) {
            s << "ERROR: extra names: " << boost::join(mismatch.extraNames, ", ") << std::endl;
            state.hasErrors = true;
        }
        if (!mismatch.missingNames.empty()) {
            s << "WARNING: missing names: " << boost::join(mismatch.missingNames, ", ") << std::endl;
            state.hasWarnings = true;
        }
        s << std::endl;
    }
};

} // namespace silver_bullets::iterate_struct
