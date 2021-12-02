#pragma once

#include "ValidatorState.hpp"
#include "TreePathTracker.hpp"

#include <boost/noncopyable.hpp>

namespace silver_bullets::iterate_struct {

template<class... Components>
struct CompositeValidator :
    public TreePathTracker,
    public boost::noncopyable
{
    using Self = CompositeValidator<Components...>;

    CompositeValidator() = default;

    explicit CompositeValidator(std::ostream* errorStream) :
        state(errorStream)
    {}

    CompositeValidator(Self&& that) :
        state(that.state),
        components(std::move(that.components))
    {
        if (state.hasErrors)
            throw std::runtime_error("Validation failure");
    }

    template<class T, class ChildNameGetter>
    void validate(const T& value, const ChildNameGetter& childNameGetter)
    {
        validate_impl(
            std::make_index_sequence<sizeof...(Components)>(),
            value,
            childNameGetter);
    }

    template<size_t... Indices, class T, class ChildNameGetter>
    void validate_impl(std::index_sequence<Indices...>,
                       const T& value,
                       const ChildNameGetter& childNameGetter)
    {
        (std::get<Indices>(components).validate(
             state, value, *this, childNameGetter), ...);
    }

    ValidatorState state;
    std::tuple<Components...> components;
};

} // namespace silver_bullets::iterate_struct
