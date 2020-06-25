#pragma once

#include "StatefulTaskFuncTraits.hpp"
#include "Task.hpp"
#include "TaskExecutorCancelParam.hpp"
#include "sync/CancelController.hpp"

#include <memory>

namespace silver_bullets {
namespace task_engine {

class StatefulCancellableTaskFuncInterface
{
public:
    virtual void call(
            const pany_range& out,
            const const_pany_range& in,
            const CancelController::Checker& isCancelled) const = 0;

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

class StatefulCancellableTaskFunc
{
public:
    StatefulCancellableTaskFunc() = default;
    explicit StatefulCancellableTaskFunc(
            const std::shared_ptr<StatefulCancellableTaskFuncInterface>& statefulTaskFunc) :
        m_statefulTaskFunc(statefulTaskFunc)
    {}
    void operator()(
            const pany_range& out,
            const const_pany_range& in,
            const CancelController::Checker& cancel) const
    {
        m_statefulTaskFunc->call(out, in, cancel);
    }

    StatefulCancellableTaskFuncInterface *func() const {
        return m_statefulTaskFunc.get();
    }

    operator bool() const {
        return !!m_statefulTaskFunc;
    }
private:
    std::shared_ptr<StatefulCancellableTaskFuncInterface> m_statefulTaskFunc;
};

template<> struct TaskFuncTraits<StatefulCancellableTaskFunc> :
        public StatefulTaskFuncTraits<StatefulCancellableTaskFunc>
{};

template<> struct IsCancellable<StatefulCancellableTaskFunc> : std::true_type {};

template<> class ThreadedTaskExecutorInit<StatefulCancellableTaskFunc> :
        public StatefulThreadedTaskExecutorInit<StatefulCancellableTaskFunc>
{
public:
    template<class F>
    explicit ThreadedTaskExecutorInit(
            const TaskFuncRegistry<StatefulCancellableTaskFunc> *taskFuncRegistry,
            const F& init,
            const TaskExecutorCancelParam_t<StatefulCancellableTaskFunc>& cp) :
        StatefulThreadedTaskExecutorInit<StatefulCancellableTaskFunc>(taskFuncRegistry, init),
        cancelParam(cp)
    {}
    TaskExecutorCancelParam_t<StatefulCancellableTaskFunc> cancelParam;
};

template<>
inline void callTaskFunc<StatefulCancellableTaskFunc>(
        const StatefulCancellableTaskFunc& f,
        const pany_range& outputs,
        const const_pany_range& inputs,
        const TaskExecutorCancelParam_t<StatefulCancellableTaskFunc>& cancelParam,
        typename TaskFuncTraits<StatefulCancellableTaskFunc>::ThreadLocalData* threadLocalData,
        const typename TaskFuncTraits<StatefulCancellableTaskFunc>::ReadOnlySharedData* readOnlySharedData)
{
    auto func = f.func();
    func->setThreadLocalData(threadLocalData);
    func->setReadOnlySharedData(readOnlySharedData);
    func->call(outputs, inputs, cancelParam);
}

} // namespace task_engine
} // namespace silver_bullets
