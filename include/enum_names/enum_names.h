#pragma once

#include <string>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/range/iterator_range.hpp>

template <class E> struct enum_names;

namespace enum_names_detail {

template <class T, std::size_t N>
inline constexpr std::size_t array_length(const std::pair<T, const char*>(&)[N]) {
    return N;
}

} // namespace enum_names_detail

template <class E>
inline constexpr std::size_t enum_item_count() {
    return enum_names_detail::array_length(enum_names<E>::names);
}

template <class E>
inline const char *enum_item_name(E e)
{
    for (auto item : enum_names<E>::names) {
        if (item.first == e)
            return item.second;
    }
    throw std::range_error(std::string("Unknown enum value ") + boost::lexical_cast<std::string>(static_cast<int>(e)));
}

template <class E>
inline E enum_item_value(const std::string& name) {
    for (auto item : enum_names<E>::names) {
        if (item.second == name)
            return item.first;
    }
    throw std::range_error(std::string("Unknown enum item name '") + name + "'");
}

template <class E>
inline constexpr const std::pair<E, const char*>* enum_item_begin() {
    return enum_names<E>::names;
}

template <class E>
inline constexpr const std::pair<E, const char*>* enum_item_end() {
    return enum_names<E>::names + enum_item_count<E>();
}

template <class E>
constexpr boost::iterator_range<const std::pair<E, const char*>*> enum_item_range() {
    return { enum_item_begin<E>(), enum_item_end<E>() };
}

#define DQR_BEGIN_DEFINE_ENUM_NAMES(EnumClassType) \
    template<> struct enum_names<EnumClassType> { \
        static constexpr const std::pair<EnumClassType, const char*> names[] = {

#define DQR_END_DEFINE_ENUM_NAMES() \
        }; \
    };
