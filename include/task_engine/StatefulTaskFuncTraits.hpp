#pragma once

#include "types.hpp"
#include <functional>

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
struct StatefulTaskFuncTraits
{
    using ThreadLocalData = boost::any;
    using ReadOnlySharedData = boost::any;

    static void setTaskFuncResources(
            ThreadLocalData* threadLocalData,
            const ReadOnlySharedData* readOnlySharedData,
            TaskFunc& taskFunc)
    {
        auto func = taskFunc.func();
        func->setThreadLocalData(threadLocalData);
        func->setReadOnlySharedData(readOnlySharedData);
    }
};

template<class TaskFunc>
class StatefulThreadedTaskExecutorInit
{
public:
    template<class F>
    explicit StatefulThreadedTaskExecutorInit(const F& init) :
        m_init(init)
    {}
    typename TaskFuncTraits<TaskFunc>::ThreadLocalData operator()() const {
        return m_init();
    }
private:
    std::function<boost::any()> m_init;
};

} // namespace task_engine
} // namespace silver_bullets
