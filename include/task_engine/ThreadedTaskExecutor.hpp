#pragma once

#include "TaskExecutor.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/assert.hpp>

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
class ThreadedTaskExecutor : public TaskExecutor<TaskFunc>
{
public:
    using Cb = typename TaskExecutor<TaskFunc>::Cb;
    using ReadOnlySharedData = typename TaskFuncTraits<TaskFunc>::ReadOnlySharedData;

    template<class ... InitArgs>
    explicit ThreadedTaskExecutor(
            int resourceType, InitArgs ... initArgs) :
        m_resourceType(resourceType),
        m_thread([this, init = ThreadedTaskExecutorInit<TaskFunc> (initArgs...)]() {
            run(init);
        })
    {}

    ~ThreadedTaskExecutor()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_flags |= ExitRequested;
        lk.unlock();
        m_cond.notify_one();
        m_thread.join();
    }

    int resourceType() const override {
        return m_resourceType;
    }

    void doStart(TaskExecutorStartParam<TaskFunc>&& startParam) override
    {
        BOOST_ASSERT(!m_f);
        m_startParam = std::move(startParam);
        m_f = m_startParam.taskFuncRegistry.get().at(m_startParam.task.taskFuncId);
        std::unique_lock<std::mutex> lk(m_mutex);
        m_flags = HasInput;
        lk.unlock();
        m_cond.notify_one();
    }

    bool propagateCb() override
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        if (m_flags & HasOutput) {
            m_flags = 0;
            if (m_startParam.cb)
                m_startParam.cb();
            m_f = TaskFunc();
            m_startParam = TaskExecutorStartParam<TaskFunc>::makeInvalidInstance();
            return true;
        }
        else
            return false;
    }

    void setReadOnlySharedData(const ReadOnlySharedData *readOnlySharedData) {
        m_readOnlySharedData = readOnlySharedData;
    }

    const ReadOnlySharedData *readOnlySharedData() const {
        return m_readOnlySharedData;
    }

private:
    int m_resourceType;
    TaskFunc m_f;
    TaskExecutorStartParam<TaskFunc> m_startParam =
            TaskExecutorStartParam<TaskFunc>::makeInvalidInstance();
    std::mutex m_mutex;
    std::condition_variable m_cond;
    enum {
        HasInput = 0x01,
        HasOutput = 0x2,
        ExitRequested = 0x4
    };
    unsigned int m_flags = 0;   // A combination of elements of the above enum
    std::thread m_thread;

    using ThreadLocalData = typename TaskFuncTraits<TaskFunc>::ThreadLocalData;

    const ReadOnlySharedData *m_readOnlySharedData = nullptr;
    ThreadLocalData m_threadLocalData;

    void run(const ThreadedTaskExecutorInit<TaskFunc>& init) {
        m_threadLocalData = init();
        while (true) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond.wait(lk, [this] {
                return !!(m_flags & (HasInput | ExitRequested));
            });
            lk.unlock();
            if (m_flags & ExitRequested)
                return;
            TaskFuncTraits<TaskFunc>::setTaskFuncResources(
                        &m_threadLocalData,
                        m_readOnlySharedData,
                        m_f);
            m_startParam.callTaskFunc(m_f);
            m_flags = HasOutput;
        }
    }
};

} // namespace task_engine
} // namespace silver_bullets
