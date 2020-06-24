#pragma once

#include "types.hpp"
#include "TaskFuncRegistry.hpp"
#include "Task.hpp"

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

template<> struct TaskExecutorStartParam<SimpleTaskFunc>
{
    Task task;
    pany_range outputs;
    const_pany_range inputs;
    std::reference_wrapper<const TaskFuncRegistry<SimpleTaskFunc>> taskFuncRegistry;
    std::function<void()> cb;

    void callTaskFunc(SimpleTaskFunc& taskFunc) const {
        taskFunc(outputs, inputs);
    }

    static TaskExecutorStartParam<SimpleTaskFunc> makeInvalidInstance()
    {
        static constexpr const TaskFuncRegistry<SimpleTaskFunc>* ptfr = nullptr;
        return {
            Task(),
            pany_range(),
            const_pany_range(),
            *ptfr,
            std::function<void()>()
        };
    }
};

template<> struct IsCancellable<SimpleTaskFunc> : std::false_type {};

} // namespace task_engine
} // namespace silver_bullets
