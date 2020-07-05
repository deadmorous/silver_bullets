#pragma once
#include <tuple>

namespace silver_bullets {
namespace templatize {

namespace detail {

template <std::size_t ... S, class T>
inline auto trunc_tuple_impl(std::integer_sequence<std::size_t, S...>, const T& t) {
    return std::make_tuple(std::get<1+S>(t)...);
}

} // detail

template <class T1, class ... Trest>
inline std::tuple<Trest ...> trunc_tuple(const std::tuple<T1, Trest ...>& t) {
    return detail::trunc_tuple_impl(std::index_sequence_for<Trest...>(), t);
}

} // namespace templatize
} // namespace silver_bullets
