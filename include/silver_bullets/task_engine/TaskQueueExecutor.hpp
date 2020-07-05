#pragma once

#include "types.hpp"
#include "TaskFuncRegistry.hpp"

#include "silver_bullets/sync/ThreadNotifier.hpp"
#include "silver_bullets/sync/CancelController.hpp"

#include <chrono>
#include <deque>
#include <thread>
#include <functional>
#include <atomic>
#include <map>

namespace silver_bullets {
namespace task_engine {

using TaskQueueFunc = std::function<void(
    boost::any& threadLocalData,
    const sync::CancelController::Checker& isCancelled)>;

using TaskQueueFuncRegistry = TaskFuncRegistry<TaskQueueFunc>;

template<> class ThreadedTaskExecutorInit<TaskQueueFunc>
{
public:
    typename boost::any operator()() const {
        return boost::any();
    }
};

struct TaskQueueExecutorStartParam
{
    int taskType;
    boost::any threadLocalData;
    sync::CancelController::Checker isCancelled;
};

template<> struct IsCancellable<TaskQueueFunc> : std::true_type {};



class TaskQueueExecutor
{
public:
    explicit TaskQueueExecutor(const sync::CancelController::Checker& cancelParam) :
        m_cancelParam(cancelParam),
        m_thread([this](){ run(); })
    {}

    ~TaskQueueExecutor()
    {
        wait();
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        m_flags |= ExitRequested;
        lk.unlock();
        m_incomingTaskNotifier.notify_one();
        m_thread.join();
    }

    void setTaskFuncRegistry(const TaskQueueFuncRegistry *taskFuncRegistry) {
        m_taskFuncRegistry = taskFuncRegistry;
    }

    const TaskQueueFuncRegistry *taskFuncRegistry() const {
        return m_taskFuncRegistry;
    }

    std::size_t post(int taskType)
    {
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        BOOST_ASSERT(!m_cancelParam);
        auto taskId = m_nextTaskId++;
        m_taskQueue.emplace_back(TaskQueueItem{ taskType, taskId });
        lk.unlock();
        m_incomingTaskNotifier.notify_one();
        return taskId;
    };


    void clearQueue()
    {
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        m_taskQueue.clear();
    }

    // Cancel all tasks that are not yet completed and have identifiers >= taskId.
    void clearQueueFrom(std::size_t taskId)
    {
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        m_taskQueue.erase(findTasksFrom(taskId), m_taskQueue.end());
    }

    void wait()
    {
        while (true) {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            auto running = !!(m_flags & TaskRunning);
            if (m_taskQueue.empty() && !running)
                return;
            lk.unlock();
            m_taskCompletionNotifier.wait();
        }
    }

    // Returns the number of tasks that are to be completed after timeout
    template< class Rep, class Period >
    std::size_t wait(const std::chrono::duration<Rep, Period>& timeout)
    {
        auto endTime = std::chrono::system_clock::now() + timeout;
        auto remainingTimeout = timeout;
        while (true) {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            auto remainingTasks = m_taskQueue.size();
            if (m_flags & TaskRunning)
                ++remainingTasks;
            if (remainingTasks == 0)
                return 0;
            lk.unlock();
            if (remainingTimeout > 0) {
                m_taskCompletionNotifier.wait_for(remainingTimeout);
                auto currentTime = std::chrono::system_clock::now();
                remainingTimeout = std::chrono::duration_cast<decltype (remainingTimeout)>(endTime - currentTime);
            }
            else
                return remainingTasks;
        }
    }

    void wait(std::size_t taskId)
    {
        while (true) {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            std::size_t remainingTasks = m_taskQueue.end() - findTasksFrom(taskId);
            if (m_flags & TaskRunning)
                ++remainingTasks;
            if (remainingTasks == 0)
                return;
            lk.unlock();
            m_taskCompletionNotifier.wait();
        }
    }

    template< class Rep, class Period >
    std::size_t wait(std::size_t taskId, const std::chrono::duration<Rep, Period>& timeout)
    {
        auto endTime = std::chrono::system_clock::now() + timeout;
        auto remainingTimeout = timeout;
        while (true) {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            std::size_t remainingTasks = m_taskQueue.end() - findTasksFrom(taskId);
            if (m_flags & TaskRunning)
                ++remainingTasks;
            if (remainingTasks == 0)
                return 0;
            lk.unlock();
            if (remainingTimeout > 0) {
                m_taskCompletionNotifier.wait_for(remainingTimeout);
                auto currentTime = std::chrono::system_clock::now();
                remainingTimeout = std::chrono::duration_cast<decltype (remainingTimeout)>(endTime - currentTime);
            }
            else
                return remainingTasks;
        }
    }

private:
    sync::CancelController::Checker m_cancelParam;

    struct TaskQueueItem
    {
        int taskType;
        std::size_t taskId;
    };

    std::deque<TaskQueueItem> m_taskQueue;
    std::size_t m_nextTaskId = 1;
    sync::ThreadNotifier m_incomingTaskNotifier;
    sync::ThreadNotifier m_taskCompletionNotifier;
    boost::any m_threadLocalData;

    const TaskQueueFuncRegistry *m_taskFuncRegistry = nullptr;

    enum {
        TaskCompleted = 0x01,
        TaskRunning = 0x02,
        ExitRequested = 0x4
    };
    volatile unsigned int m_flags = 0;   // A combination of elements of the above enum

    // Note: Declare the thread last, such that all fields it can access
    // are initialized before the thread starts.
    std::thread m_thread;

    std::deque<TaskQueueItem>::iterator findTasksFrom(std::size_t taskId)
    {
        return std::lower_bound(
                    m_taskQueue.begin(),
                    m_taskQueue.end(),
                    taskId,
                    [](const TaskQueueItem& a, const std::size_t& b)
        {
            return a.taskId < b;
        });
    }

    void run()
    {
        while (true) {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            if (m_flags & ExitRequested)
                return;
            else if (m_cancelParam) {
                m_taskQueue.clear();
                m_flags = TaskCompleted;
                lk.unlock();
                m_taskCompletionNotifier.notify_all();
            }
            else if (!m_taskQueue.empty()) {
                auto task = m_taskQueue.front();
                m_flags = TaskRunning;
                m_taskQueue.pop_front();
                lk.unlock();
                auto& taskFunc = m_taskFuncRegistry->at(task.taskType);
                taskFunc(m_threadLocalData, m_cancelParam);

                lk.lock();
                m_flags = TaskCompleted;
                lk.unlock();
                m_taskCompletionNotifier.notify_all();
            }
            else {
                lk.unlock();
                m_incomingTaskNotifier.wait();
            }
        }
    }
};

} // namespace task_engine
} // namespace silver_bullets
