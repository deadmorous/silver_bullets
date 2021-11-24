#pragma once

#include "iterate_struct.hpp"
#include "tree_path_tracker.hpp"

#include <sstream>
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

class StructFieldsMismatchValidator : public TreePathTracker, public boost::noncopyable
{
public:
    StructFieldsMismatchValidator() = default;

    // TODO: handle warnings better
    StructFieldsMismatchValidator(bool throwing) : m_throwing(throwing) {}

    StructFieldsMismatchValidator(StructFieldsMismatchValidator&& that) :
        m_mismatches(std::move(that.m_mismatches)),
        m_throwing(that.m_throwing)
    {
        if (m_throwing && *this)
        {
            std::ostringstream s;
            s << std::endl;
            report(s);
            throw std::runtime_error(s.str());
        }
    }

    template<class T, class ChildNameGetter>
    void validate(const T& value, const ChildNameGetter& childNameGetter) {
        if constexpr(has_iterate_struct_helper_v<T>) {
            if (auto mismatch = struct_fields_mismatch(value, childNameGetter))
                m_mismatches[boost::join(path(), ".")] = mismatch;
        }
    }

    const std::map<std::string, StructFieldsMismatch>& mismatches() const {
        return m_mismatches;
    }

    operator bool() const {
        return !m_mismatches.empty();
    }

    void report(std::ostream& s) const
    {
        for (auto& [path, mismatch] : m_mismatches) {
            s << "At path '" << path << "':" << std::endl;
            if (!mismatch.extraNames.empty())
                s << "ERROR: extra names: " << boost::join(mismatch.extraNames, ", ") << std::endl;
            if (!mismatch.missingNames.empty())
                s << "WARNING: missing names: " << boost::join(mismatch.missingNames, ", ") << std::endl;
            s << std::endl;
        }
    }

private:
    std::map<std::string, StructFieldsMismatch> m_mismatches;
    bool m_throwing = true;
};

inline std::ostream& operator <<(std::ostream& s, const StructFieldsMismatchValidator& v)
{
    v.report(s);
    return s;
}

} // namespace silver_bullets::iterate_struct
