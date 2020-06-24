#pragma once

#include "StatefulTaskFuncTraits.hpp"
#include "TaskFuncRegistry.hpp"
#include "Task.hpp"

#include <memory>

namespace silver_bullets {
namespace task_engine {

class StatefulTaskFuncInterface
{
public:
    virtual void call(const pany_range& out, const const_pany_range& in) const = 0;

    void setThreadLocalData(boost::any *threadLocalData) {
        m_threadLocalData = threadLocalData;
    }

    boost::any *threadLocalData() const {
        return m_threadLocalData;
    }

    void setReadOnlySharedData(const boost::any *readOnlySharedData) {
        m_readOnlySharedData = readOnlySharedData;
    }

    const boost::any *readOnlySharedData() const {
        return m_readOnlySharedData;
    }

private:
    boost::any *m_threadLocalData = nullptr;
    const boost::any *m_readOnlySharedData = nullptr;
};

class StatefulTaskFunc
{
public:
    StatefulTaskFunc() = default;
    explicit StatefulTaskFunc(const std::shared_ptr<StatefulTaskFuncInterface>& statefulTaskFunc) :
        m_statefulTaskFunc(statefulTaskFunc)
    {}
    void operator()(const pany_range& out, const const_pany_range& in) {
        m_statefulTaskFunc->call(out, in);
    }
    StatefulTaskFuncInterface *func() {
        return m_statefulTaskFunc.get();
    }
    operator bool() const {
        return !!m_statefulTaskFunc;
    }
private:
    std::shared_ptr<StatefulTaskFuncInterface> m_statefulTaskFunc;
};

template<> struct TaskFuncTraits<StatefulTaskFunc> :
        StatefulTaskFuncTraits<StatefulTaskFunc>
{};

template<> class ThreadedTaskExecutorInit<StatefulTaskFunc> :
        public StatefulThreadedTaskExecutorInit<StatefulTaskFunc>
{
public:
    template<class F>
    explicit ThreadedTaskExecutorInit(const F& init) :
        StatefulThreadedTaskExecutorInit<StatefulTaskFunc>(init)
    {}
};

template<> struct TaskExecutorStartParam<StatefulTaskFunc>
{
    Task task;
    pany_range outputs;
    const_pany_range inputs;
    // const TaskFuncRegistry<StatefulTaskFunc>* taskFuncRegistry = nullptr;
    std::reference_wrapper<const TaskFuncRegistry<StatefulTaskFunc>> taskFuncRegistry;
    std::function<void()> cb;

    void callTaskFunc(StatefulTaskFunc& taskFunc) const {
        taskFunc(outputs, inputs);
    }

    static TaskExecutorStartParam<StatefulTaskFunc> makeInvalidInstance()
    {
        static constexpr const TaskFuncRegistry<StatefulTaskFunc>* ptfr = nullptr;
        return {
            Task(),
            pany_range(),
            const_pany_range(),
            *ptfr,
            std::function<void()>()
        };
    }
};

template<> struct IsCancellable<StatefulTaskFunc> : std::false_type {};

} // namespace task_engine
} // namespace silver_bullets
