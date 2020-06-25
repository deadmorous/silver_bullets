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

using StatefulCancellableTaskFunc = std::shared_ptr<StatefulCancellableTaskFuncInterface>;

template <>
struct ThreadLocalData<StatefulCancellableTaskFunc> {
    using type = boost::any;
};

template <>
struct ReadOnlySharedData<StatefulCancellableTaskFunc> {
    using type = boost::any;
};

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
        ThreadLocalData_t<StatefulCancellableTaskFunc>* threadLocalData,
        const ReadOnlySharedData_t<StatefulCancellableTaskFunc>* readOnlySharedData)
{
    auto fp = f.get();
    fp->setThreadLocalData(threadLocalData);
    fp->setReadOnlySharedData(readOnlySharedData);
    fp->call(outputs, inputs, cancelParam);
}

} // namespace task_engine
} // namespace silver_bullets
