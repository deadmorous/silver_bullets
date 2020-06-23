#pragma once

#include <functional>

namespace silver_bullets {

// See http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda

template< class T >
struct function_traits : public function_traits<decltype(&T::operator())> {};

template< class C, class R, class... Args >
struct function_traits<R(C::*)(Args...) const> {
    typedef R return_type;
    typedef std::tuple<Args...> args_type;
    static const size_t arity = sizeof...(Args);
    typedef std::function<R(Args...)> std_function_type;
};

} // namespace silver_bullets
