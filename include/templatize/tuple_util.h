#pragma once
#include <tuple>

namespace tuple_util {
namespace detail {

template <std::size_t ... S, class T>
inline auto trunc_tuple(std::integer_sequence<std::size_t, S...>, const T& t) {
    return std::make_tuple(std::get<1+S>(t)...);
}

} // detail
} // tuple_util

template <class T1, class ... Trest>
inline std::tuple<Trest ...> trunc_tuple(const std::tuple<T1, Trest ...>& t) {
    namespace d = tuple_util::detail;
    return d::trunc_tuple(std::index_sequence_for<Trest...>(), t);
}
