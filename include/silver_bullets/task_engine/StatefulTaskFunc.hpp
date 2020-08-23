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
    virtual void call(
            boost::any& threadLocalData,
            const pany_range& out,
            const const_pany_range& in) const = 0;

    void setReadOnlySharedData(const boost::any *readOnlySharedData) {
        m_readOnlySharedData = readOnlySharedData;
    }

    const boost::any *readOnlySharedData() const {
        return m_readOnlySharedData;
    }

private:
    const boost::any *m_readOnlySharedData = nullptr;
};

using StatefulTaskFunc = std::shared_ptr<StatefulTaskFuncInterface>;

template <>
struct ThreadLocalData<StatefulTaskFunc> {
    using type = boost::any;
};

template <>
struct ReadOnlySharedData<StatefulTaskFunc> {
    using type = boost::any;
};

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
        ThreadLocalData_t<StatefulTaskFunc>* threadLocalData,
        const ReadOnlySharedData_t<StatefulTaskFunc>* readOnlySharedData)
{
    auto fp = f.get();
    fp->setReadOnlySharedData(readOnlySharedData);
    fp->call(*threadLocalData, outputs, inputs);
}

} // namespace task_engine
} // namespace silver_bullets
