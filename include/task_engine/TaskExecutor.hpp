#pragma once

#include "types.hpp"
#include "Task.hpp"
#include "TaskFuncRegistry.hpp"

#include <functional>

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
class TaskExecutor
{
public:
    using Cb = std::function<void()>;

    virtual ~TaskExecutor() = default;
    virtual int resourceType() const = 0;
    virtual void start(
            const Task& task,
            const pany_range& outputs,
            const const_pany_range& inputs,
            const Cb& cb,
            const TaskFuncRegistry<TaskFunc>& taskFuncRegistry) = 0;
    virtual bool propagateCb() = 0;
};

} // namespace task_engine
} // namespace silver_bullets
