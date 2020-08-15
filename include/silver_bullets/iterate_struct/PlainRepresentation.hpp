#pragma once

#include <type_traits>

namespace silver_bullets {
namespace iterate_struct {

template<class T> struct PlainRepresentation {
    using type = T;
};
template<class T> using PlainRepresentation_t = typename PlainRepresentation<T>::type;

template<class T>
inline PlainRepresentation_t<T> toPlain(
    const T& x,
    std::enable_if_t<std::is_same_v<T,PlainRepresentation_t<T>>, int> = 0)
{
    return x;
}

template<class T>
inline T fromPlain(
    const PlainRepresentation_t<T>& x,
    std::enable_if_t<std::is_same_v<T,PlainRepresentation_t<T>>, int> = 0)
{
    return x;
}

template<class T>
inline PlainRepresentation_t<T> toPlain(
    const T& x,
    std::enable_if_t<!std::is_same_v<T,PlainRepresentation_t<T>>, int> = 0)
{
    return PlainRepresentation_t<T>::toPlain(x);
}

template<class T>
inline T fromPlain(
    const PlainRepresentation_t<T>& x,
    std::enable_if_t<!std::is_same_v<T,PlainRepresentation_t<T>>, int> = 0)
{
    return PlainRepresentation_t<T>::fromPlain(x);
}

} // namespace iterate_struct
} // namespace silver_bullets
