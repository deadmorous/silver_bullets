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

namespace detail {

template<class T>
static auto test_validate(int)
    -> sfinae_true<decltype(validate(std::declval<T>()))>;
template<class>
static auto test_validate(long) -> std::false_type;

template<class T>
struct has_validate : decltype(detail::test_validate<T>(0)){};

template<class T>
void call_validate(const T& x)
{
    static_assert (has_validate<T>::value, "Trying to call `validate` that is not provided for this type");
    validate(x);
}

} // detail


class StructDataValidator
{
public:
    template<class T, class ChildNameGetter>
    void validate(ValidatorState& state,
                  const T& value,
                  const TreePathTracker& pathGetter,
                  const ChildNameGetter& /*childNameGetter*/)
    {
        if constexpr(detail::has_validate<T>::value)
            try {
                detail::call_validate(value);
            }
            catch(const std::exception& e) {
                *state.errorStream
                    << "At path '" << boost::join(pathGetter.path(), ".") << "':" << std::endl
                    << "ERROR: " << e.what() << std::endl;
                state.hasErrors = true;
            }
    }
};

} // namespace silver_bullets::iterate_struct
