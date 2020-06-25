#pragma once

#include "StatefulTaskFuncTraits.hpp"
#include "TaskExecutorCancelParam.hpp"
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
    void operator()(const pany_range& out, const const_pany_range& in) const {
        m_statefulTaskFunc->call(out, in);
    }
    StatefulTaskFuncInterface *func() const {
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

template<> struct IsCancellable<StatefulTaskFunc> : std::false_type {};

template<> class ThreadedTaskExecutorInit<StatefulTaskFunc> :
        public StatefulThreadedTaskExecutorInit<StatefulTaskFunc>
{
public:
    template<class F>
    explicit ThreadedTaskExecutorInit(
            const TaskFuncRegistry<StatefulTaskFunc> *taskFuncRegistry,
            const F& init) :
        StatefulThreadedTaskExecutorInit<StatefulTaskFunc>(taskFuncRegistry, init)
    {}
    TaskExecutorCancelParam_t<StatefulTaskFunc> cancelParam;
};

template<>
inline void callTaskFunc<StatefulTaskFunc>(
        const StatefulTaskFunc& f,
        const pany_range& outputs,
        const const_pany_range& inputs,
        const TaskExecutorCancelParam_t<StatefulTaskFunc>&,
        typename TaskFuncTraits<StatefulTaskFunc>::ThreadLocalData* threadLocalData,
        const typename TaskFuncTraits<StatefulTaskFunc>::ReadOnlySharedData* readOnlySharedData)
{
    auto func = f.func();
    func->setThreadLocalData(threadLocalData);
    func->setReadOnlySharedData(readOnlySharedData);
    func->call(outputs, inputs);
}

} // namespace task_engine
} // namespace silver_bullets
