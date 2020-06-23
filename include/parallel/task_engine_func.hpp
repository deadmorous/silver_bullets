#pragma once

#include "task_engine.hpp"
#include "func/func_arg_convert.hpp"

namespace silver_bullets {

namespace detail {

struct FromAnyPtr
{
    template<class T>
    const T& operator()(const boost::any* src) const {
        return boost::any_cast<const T&>(*src);
    }
};

struct ToAnyPtrRange
{
    template<class T>
    void operator()(const pany_range& dst, const T& src) const {
        *(dst[0]) = src;
    }
};

} // namespace detail

template<class F> inline TaskFunc makeTaskFunc(F f)
{
    return func_arg_convert<const pany_range&, const const_pany_range&>(
                f, detail::ToAnyPtrRange(), detail::FromAnyPtr());
}


} // namespace silver_bullets
