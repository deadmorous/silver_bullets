#pragma once

#include "types.hpp"
#include <functional>
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

template<>
struct TaskFuncTraits<StatefulTaskFunc>
{
    using ThreadLocalData = boost::any;
    using ReadOnlySharedData = boost::any;

    static void setTaskFuncResources(
            ThreadLocalData* threadLocalData,
            const ReadOnlySharedData* readOnlySharedData,
            StatefulTaskFunc& taskFunc)
    {
        auto func = taskFunc.func();
        func->setThreadLocalData(threadLocalData);
        func->setReadOnlySharedData(readOnlySharedData);
    }
};

template<> class ThreadedTaskExecutorInit<StatefulTaskFunc>
{
public:
    template<class F>
    explicit ThreadedTaskExecutorInit(const F& init) :
        m_init(init)
    {}
    TaskFuncTraits<StatefulTaskFunc>::ThreadLocalData operator()() const {
        return m_init();
    }
private:
    std::function<boost::any()> m_init;
};

} // namespace task_engine
} // namespace silver_bullets
