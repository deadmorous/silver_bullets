#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <vector>
#include <map>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/version.hpp>

namespace silver_bullets {
namespace iterate_struct {

template <class S> class iterate_struct_helper;

namespace detail {

// See https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
// ---- 8< ----
template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if_t<I == sizeof...(Tp), void>
for_each_with_index(std::tuple<Tp...>&&, FuncT) // Unused arguments are given no names.
{
}

template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if_t<I < sizeof...(Tp), void>
for_each_with_index(std::tuple<Tp...>&& t, FuncT f)
{
    f(std::get<I>(t), I);
    for_each_with_index<I + 1, FuncT, Tp...>(std::forward<std::tuple<Tp...>>(t), f);
}
// ---- 8< ----

// https://stackoverflow.com/questions/257288/is-it-possible-to-write-a-template-to-check-for-a-functions-existence
template<class>
struct sfinae_true : std::true_type{};

template<class T, class A0>
static auto test_input_operator(int)
  -> sfinae_true<decltype(std::declval<T>() >> (std::declval<A0>()))>;
template<class, class>
static auto test_input_operator(long) -> std::false_type;

template<class T, class A0>
static auto test_output_operator(int)
  -> sfinae_true<decltype(std::declval<T>() << (std::declval<A0>()))>;
template<class, class>
static auto test_output_operator(long) -> std::false_type;

template<class T>
static auto test_iterate_struct_helper(int)
  -> sfinae_true<decltype(iterate_struct_helper<T>::asTuple(std::declval<T>()))>;
template<class>
static auto test_iterate_struct_helper(long) -> std::false_type;

} // namespace detail

template<class T, class Arg>
struct has_input_operator : decltype(detail::test_input_operator<T, Arg>(0)){};
template<class T, class Arg>
constexpr auto has_input_operator_v = has_input_operator<T, Arg>::value;

template<class T, class Arg>
struct has_output_operator : decltype(detail::test_output_operator<T, Arg>(0)){};
template<class T, class Arg>
constexpr auto has_output_operator_v = has_output_operator<T, Arg>::value;

template<class T>
struct has_iterate_struct_helper : decltype(detail::test_iterate_struct_helper<T>(0)){};
template<class T>
constexpr auto has_iterate_struct_helper_v = has_iterate_struct_helper<T>::value;

template<class T>
struct is_vector : std::false_type {};

template<class T>
struct is_vector<std::vector<T>> : std::true_type {};

template<class T>
constexpr auto is_vector_v = is_vector<T>::value;

template<class T>
struct is_pair : std::false_type {};

template<class T1, class T2>
struct is_pair<std::pair<T1, T2>> : std::true_type {};

template<class T>
constexpr auto is_pair_v = is_pair<T>::value;

template<class T>
struct is_map : std::false_type {};

template<class T1, class T2>
struct is_map<std::map<T1, T2>> : std::true_type {};

template<class T>
constexpr auto is_map_v = is_map<T>::value;

template <class NamedFieldVisitor>
struct IndexedFieldVisitor
{
    explicit IndexedFieldVisitor(const NamedFieldVisitor& namedVisitor, const char* const* names) :
        m_namedVisitor(namedVisitor),
        m_names(names) {}
    template<class T>
    void operator()(T& value, unsigned int index) const {
        m_namedVisitor(value, m_names[index]);
    }
    const NamedFieldVisitor& m_namedVisitor;
    const char* const* m_names;
};

template<class S, class NamedFieldVisitor>
void for_each(S& s, const NamedFieldVisitor& visitor)
{
    using ST = iterate_struct_helper<std::decay_t<S>>;
    detail::for_each_with_index(ST::asTuple(s), IndexedFieldVisitor(visitor, ST::fieldNames));
}

template <class S>
inline auto asTuple(S& s, std::enable_if_t<has_iterate_struct_helper_v<std::decay_t<S>>, int> = 0) {
    return iterate_struct_helper<std::decay_t<S>>::asTuple(s);
}

} // namespace iterate_struct
} // namespace silver_bullets

// NOTE
//  In Boost up to 1.74, BOOST_PP_SEQ_FOR_EACH passes 2 as first element index,
//  whereas in later Boost versions (starting from 1.75), it's 1.
#if BOOST_VERSION < 107500
#define SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX 2
#else // BOOST_VERSION
#define SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX 1
#endif // BOOST_VERSION

#define SILVER_BULLETS_ITERATE_STRUCT_SEQ_ITEM_INDEX(R, _, __) R
static_assert(BOOST_PP_SEQ_FOR_EACH(SILVER_BULLETS_ITERATE_STRUCT_SEQ_ITEM_INDEX, _, ()) ==
              SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX,
              "Mismatch of guessed and actual first sequence item index when "
              "using BOOST_PP_SEQ_FOR_EACH. "
              "Update SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX definition");

// See https://stackoverflow.com/questions/27765387/distributing-an-argument-in-a-variadic-macro
#define SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELD(r, instance, field)      \
    BOOST_PP_COMMA_IF(                                                      \
        BOOST_PP_SUB(r, SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX))\
    instance.field

#define SILVER_BULLETS_ITERATE_STRUCT_ACCESS_SOME_FIELDS(instance, ...) \
    BOOST_PP_SEQ_FOR_EACH(SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELD, \
        instance, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SILVER_BULLETS_ITERATE_STRUCT_ACCESS_NO_FIELDS(...)

#define SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELDS(instance, ...) \
    BOOST_PP_IF(BOOST_PP_SUB(BOOST_PP_VARIADIC_SIZE(0, ##__VA_ARGS__), 1), \
                SILVER_BULLETS_ITERATE_STRUCT_ACCESS_SOME_FIELDS, \
                SILVER_BULLETS_ITERATE_STRUCT_ACCESS_NO_FIELDS)(instance, ##__VA_ARGS__)

// https://stackoverflow.com/questions/27765387/distributing-an-argument-in-a-variadic-macro
#define SILVER_BULLETS_ITERATE_STRUCT_SIMPLE_STRINGIZE(x) #x
#define SILVER_BULLETS_ITERATE_STRUCT_STRINGIZE_FIELD(r, data, field)       \
    BOOST_PP_COMMA_IF(                                                      \
        BOOST_PP_SUB(r, SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX))\
    SILVER_BULLETS_ITERATE_STRUCT_SIMPLE_STRINGIZE(field)

#define SILVER_BULLETS_ITERATE_STRUCT_STRINGIZE_FIELDS(...) \
    BOOST_PP_SEQ_FOR_EACH(SILVER_BULLETS_ITERATE_STRUCT_STRINGIZE_FIELD, \
        UNUSED, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(Struct, ...) \
    namespace silver_bullets { \
    namespace iterate_struct { \
        template <> class iterate_struct_helper<Struct> { public: \
            using S = Struct; \
            static auto asTuple(S& s) { \
                (void)s; \
                return std::forward_as_tuple(SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELDS(s, ##__VA_ARGS__)); \
            } \
            static auto asTuple(const S& s) { \
                (void)s; \
                return std::forward_as_tuple(SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELDS(s, ##__VA_ARGS__)); \
            } \
            static constexpr const char *fieldNames[] = { \
                SILVER_BULLETS_ITERATE_STRUCT_STRINGIZE_FIELDS(__VA_ARGS__) \
            }; \
        }; \
    } }



#define SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_PARAMETER(r, data, elem)     \
    BOOST_PP_COMMA_IF(                                                      \
        BOOST_PP_SUB(r, SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX))\
    BOOST_PP_TUPLE_ELEM(2, 0, elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)

#define SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_PARAMETERS(seq) \
    BOOST_PP_SEQ_FOR_EACH(SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_PARAMETER, _, seq)

#define SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_ARGUMENT(r, data, elem)      \
    BOOST_PP_COMMA_IF(                                                      \
        BOOST_PP_SUB(r, SILVER_BULLETS_ITERATE_STRUCT_FIRST_SEQ_ITEM_INDEX))\
    BOOST_PP_TUPLE_ELEM(2, 1, elem)

#define SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_ARGUMENTS(seq) \
    BOOST_PP_SEQ_FOR_EACH(SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_ARGUMENT, _, seq)

#define SILVER_BULLETS_DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(TemplateParameters, Struct, ...) \
    namespace silver_bullets { \
    namespace iterate_struct { \
        template <SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_PARAMETERS(BOOST_PP_TUPLE_TO_SEQ(TemplateParameters))> \
        class iterate_struct_helper<Struct<SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_ARGUMENTS(BOOST_PP_TUPLE_TO_SEQ(TemplateParameters))>> { public: \
            using S = Struct<SILVER_BULLETS_ITERATE_STRUCT_TEMPLATE_ARGUMENTS(BOOST_PP_TUPLE_TO_SEQ(TemplateParameters))>; \
            static auto asTuple(S& s) { \
                (void)s; \
                return std::forward_as_tuple(SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELDS(s, ##__VA_ARGS__)); \
            } \
            static auto asTuple(const S& s) { \
                (void)s; \
                return std::forward_as_tuple(SILVER_BULLETS_ITERATE_STRUCT_ACCESS_FIELDS(s, ##__VA_ARGS__)); \
            } \
            static constexpr const char *fieldNames[] = { \
                SILVER_BULLETS_ITERATE_STRUCT_STRINGIZE_FIELDS(__VA_ARGS__) \
            }; \
        }; \
    } }
