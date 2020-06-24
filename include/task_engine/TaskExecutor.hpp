#pragma once

#include "types.hpp"
#include "Task.hpp"
#include "TaskFuncRegistry.hpp"

#include <functional>

namespace silver_bullets {

class ThreadNotifier;

namespace task_engine {

template<class TaskFunc>
class TaskExecutor
{
public:
    using Cb = std::function<void()>;

    virtual ~TaskExecutor() = default;
    virtual int resourceType() const = 0;
    virtual bool propagateCb() = 0;
    virtual void setTaskCompletionNotifier(ThreadNotifier *taskCompletionNotifier) = 0;
    virtual ThreadNotifier *taskCompletionNotifier() const = 0;

    template<class ... Args>
    void start(
            const Task& task,
            const pany_range& outputs,
            const const_pany_range& inputs,
            const TaskFuncRegistry<TaskFunc>& taskFuncRegistry,
            Args&& ... args) {
        doStart({ task, outputs, inputs, taskFuncRegistry, std::forward<Args>(args)... });
    }

    void start(TaskExecutorStartParam<TaskFunc>&& startParam) {
        doStart(std::move(startParam));
    }

protected:
    virtual void doStart(TaskExecutorStartParam<TaskFunc>&& startParam) = 0;
};

} // namespace task_engine
} // namespace silver_bullets
