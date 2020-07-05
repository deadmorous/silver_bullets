#pragma once

#include <cstddef>
#include <tuple>

namespace silver_bullets {

namespace func {
namespace detail {

template<class T, std::size_t i> struct Indexed {
    using type = T;
    static constexpr const std::size_t index = i;
};

template <std::size_t N, class ... T> struct OffsetIndexedTuple;

template<std::size_t N, class T> struct OffsetIndexedTuple<N, T> {
    using type = std::tuple<Indexed<T, N>>;
};

// https://stackoverflow.com/questions/53394100/concatenating-tuples-as-types
template <class, class> struct Cat;
template <class... First, class... Second>
struct Cat<std::tuple<First...>, std::tuple<Second...>> {
    using type = std::tuple<First..., Second...>;
};

template<std::size_t N, class T, class ... Rest>
struct OffsetIndexedTuple<N, T, Rest ...> {
    using type = typename Cat<
        typename OffsetIndexedTuple<N, T>::type,
        typename OffsetIndexedTuple<N+1, Rest...>::type >::type;
};

} // namespace detail
} // namespace func

template<class> struct indexed_tuple;
template<class ... T> struct indexed_tuple<std::tuple<T...>>
{
    using type = typename func::detail::OffsetIndexedTuple<0, T...>::type;
};

template<class ... T>
using indexed_tuple_t = typename indexed_tuple<T...>::type;

} // namespace silver_bullets
