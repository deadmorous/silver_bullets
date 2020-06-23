#pragma once

#include "types.hpp"

#include "func/func_arg_convert.hpp"

#include <functional>

namespace silver_bullets {
namespace task_engine {

using SimpleTaskFunc = std::function<void(const pany_range&, const const_pany_range&)>;



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

template<class F> inline SimpleTaskFunc makeSimpleTaskFunc(F f)
{
    return func_arg_convert<const pany_range&, const const_pany_range&>(
                f, detail::ToAnyPtrRange(), detail::FromAnyPtr());
}



template<>
struct TaskFuncTraits<SimpleTaskFunc>
{
    struct ThreadLocalData {};
    struct ReadOnlySharedData {};
    static void setTaskFuncResources(
            ThreadLocalData*,
            const ReadOnlySharedData*,
            SimpleTaskFunc&)
    {}
};



template<> class ThreadedTaskExecutorInit<SimpleTaskFunc>
{
public:
    ThreadedTaskExecutorInit() = default;
    TaskFuncTraits<SimpleTaskFunc>::ThreadLocalData operator()() const {
        return {};
    }
};

} // namespace task_engine
} // namespace silver_bullets
