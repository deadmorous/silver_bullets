#pragma once

#include "StatefulTaskFuncTraits.hpp"
#include "TaskFuncRegistry.hpp"
#include "Task.hpp"

#include <memory>
#include <atomic>

namespace silver_bullets {
namespace task_engine {

class StatefulCancellableTaskFuncInterface
{
public:
    virtual void call(
            const pany_range& out,
            const const_pany_range& in,
            const std::atomic<bool>& cancel) const = 0;

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
    explicit StatefulCancellableTaskFunc(const std::shared_ptr<StatefulCancellableTaskFuncInterface>& statefulTaskFunc) :
        m_statefulTaskFunc(statefulTaskFunc)
    {}
    void operator()(
            const pany_range& out,
            const const_pany_range& in,
            const std::atomic<bool>& cancel)
    {
        m_statefulTaskFunc->call(out, in, cancel);
    }

    StatefulCancellableTaskFuncInterface *func() {
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

template<> class ThreadedTaskExecutorInit<StatefulCancellableTaskFunc> :
        public StatefulThreadedTaskExecutorInit<StatefulCancellableTaskFunc>
{
public:
    template<class F>
    explicit ThreadedTaskExecutorInit(const F& init) :
        StatefulThreadedTaskExecutorInit<StatefulCancellableTaskFunc>(init)
    {}
};

template<> struct TaskExecutorStartParam<StatefulCancellableTaskFunc>
{
    Task task;
    pany_range outputs;
    const_pany_range inputs;
    std::reference_wrapper<const TaskFuncRegistry<StatefulCancellableTaskFunc>> taskFuncRegistry;
    std::reference_wrapper<std::atomic<bool>> cancel;
    std::function<void()> cb;

    void callTaskFunc(StatefulCancellableTaskFunc& taskFunc) const {
        taskFunc(outputs, inputs, cancel);
    }

    static TaskExecutorStartParam<StatefulCancellableTaskFunc> makeInvalidInstance()
    {
        static constexpr const TaskFuncRegistry<StatefulCancellableTaskFunc>* ptfr = nullptr;
        static constexpr std::atomic<bool> *pcancel = nullptr;
        return {
            Task(),
            pany_range(),
            const_pany_range(),
            *ptfr,
            *pcancel,
            std::function<void()>()
        };
    }
};

template<> struct IsCancellable<StatefulCancellableTaskFunc> : std::true_type {};

} // namespace task_engine
} // namespace silver_bullets
