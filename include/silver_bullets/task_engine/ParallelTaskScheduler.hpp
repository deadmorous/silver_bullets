#pragma once

#include "TaskExecutor.hpp"

#include "silver_bullets/sync/ThreadNotifier.hpp"

#include <memory>
#include <vector>
#include <deque>

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
class ParallelTaskScheduler
{
public:
    template<class ... Args>
    explicit ParallelTaskScheduler(Args&& ... args) :
        m_cancelParam(std::forward<Args>(args)...)
    {
    }

    ParallelTaskScheduler& addTaskExecutor(const std::shared_ptr<TaskExecutor<TaskFunc>>& taskExecutor)
    {
        m_resourceInfo[taskExecutor->resourceType()].executors.push_back(taskExecutor);
        taskExecutor->setTaskCompletionNotifier(&m_taskCompletionNotifier);
        return *this;
    }

    size_t addTask(const TaskExecutorStartParam& startParam)
    {
        auto& ri = m_resourceInfo.at(startParam.task.resourceType);
        ri.tasks.push_back(startParam);
        maybeStartNextTask(startParam.task.resourceType);
    }

    bool isRunning() const {
        return m_running;
    }

    ParallelTaskScheduler& wait()
    {
        while(m_running) {
            m_taskCompletionNotifier.wait();
            propagateCb();
        }
        return *this;
    }

    template< class Rep, class Period >
    bool maybeWait(const std::chrono::duration<Rep, Period>& timeout)
    {
        auto endTime = std::chrono::system_clock::now() + timeout;
        auto remainingTimeout = timeout;
        while(m_running) {
            if (!m_taskCompletionNotifier.wait_for(remainingTimeout))
                return false;
            auto currentTime = std::chrono::system_clock::now();
            if (currentTime >= endTime)
                return false;
            remainingTimeout = std::chrono::duration_cast<decltype (remainingTimeout)>(endTime - currentTime);
            propagateCb();
        }
        return true;
    }

    bool propagateCb()
    {
        auto cancelled = TaskExecutorCancelParam<TaskFunc>::isCancelled(m_cancelParam);
        if (m_running) {
            // Track finished tasks
            std::size_t totalRunningExecutorCount = 0;
            for (auto& resourceInfoItem : m_resourceInfo) {
                auto& ri = resourceInfoItem.second;
                if (cancelled)
                    ri.tasks.clear();
                for (std::size_t i=0; i<ri.runningExecutorCount; ++i) {
                    auto& x = ri.executors[i];
                    if (x->propagateCb()) {
                        // Executor has finished task

                        // Put the executor after the last currently executing one
                        BOOST_ASSERT(ri.runningExecutorCount > 0);
                        --ri.runningExecutorCount;
                        if (i != ri.runningExecutorCount) {
                            BOOST_ASSERT(i < ri.runningExecutorCount);
                            std::swap(x, ri.executors[ri.runningExecutorCount]);
                            --i;
                        }

                        // Start next task, if any
                        if (!cancelled)
                            maybeStartNextTask(resourceInfoItem.first);
                    }
                }
                totalRunningExecutorCount += ri.runningExecutorCount;
            }
            if (totalRunningExecutorCount == 0)
                m_running = false;
        }
        return !m_running;
    }

    TaskExecutorCancelParam_t<TaskFunc>& cancelParam() {
        return m_cancelParam;
    }

private:
    struct ResourceInfo
    {
        std::vector<std::shared_ptr<TaskExecutor<TaskFunc>>> executors;
        std::size_t runningExecutorCount = 0;   // Running are all at the beginning of executors
        std::deque <TaskExecutorStartParam> tasks;
    };

    TaskExecutorCancelParam_t<TaskFunc> m_cancelParam;

    sync::ThreadNotifier m_taskCompletionNotifier;
    std::map<int, ResourceInfo> m_resourceInfo;

    bool m_running = false;

    bool maybeStartNextTask(int resourceType)
    {
        auto& ri = m_resourceInfo.at(resourceType);
        if (ri.tasks.empty())
            return false;

        if (ri.executors.empty())
            throw std::runtime_error("ParallelTaskScheduler: No suitable resources are supplied");

        if (ri.runningExecutorCount < ri.executors.size()) {
            // Start next task
            auto& x = ri.executors[ri.runningExecutorCount];
            auto startParam = ri.tasks.front();
            ri.tasks.pop_front();
            x->start(std::move(startParam));
            ++ri.runningExecutorCount;
            m_running = true;
            return true;
        }
        else {
            // All executors are busy
            BOOST_ASSERT(m_running);
            return false;
        }
    }
};

} // namespace task_engine
} // namespace silver_bullets
