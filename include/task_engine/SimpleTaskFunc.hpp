#pragma once

#include "types.hpp"
#include "TaskFuncRegistry.hpp"
#include "Task.hpp"
#include "TaskExecutorCancelParam.hpp"

#include "func/func_arg_convert.hpp"

#include <functional>
#include <optional>

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
};

template<> struct IsCancellable<SimpleTaskFunc> : std::false_type {};

template<> class ThreadedTaskExecutorInit<SimpleTaskFunc>
{
public:
    ThreadedTaskExecutorInit(const TaskFuncRegistry<SimpleTaskFunc> *taskFuncRegistry) :
        taskFuncRegistry(taskFuncRegistry)
    {}
    TaskFuncTraits<SimpleTaskFunc>::ThreadLocalData initThreadLocalData() const {
        return {};
    }
    const TaskFuncRegistry<SimpleTaskFunc> *taskFuncRegistry;
    TaskExecutorCancelParam_t<SimpleTaskFunc> cancelParam;
};

template<>
inline void callTaskFunc<SimpleTaskFunc>(
        const SimpleTaskFunc& f,
        const pany_range& outputs,
        const const_pany_range& inputs,
        const TaskExecutorCancelParam_t<SimpleTaskFunc>&,
        typename TaskFuncTraits<SimpleTaskFunc>::ThreadLocalData*,
        const typename TaskFuncTraits<SimpleTaskFunc>::ReadOnlySharedData*)
{
    f(outputs, inputs);
}

} // namespace task_engine
} // namespace silver_bullets
