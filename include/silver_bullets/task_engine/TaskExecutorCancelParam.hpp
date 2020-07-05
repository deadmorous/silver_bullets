#pragma once

#include "types.hpp"
#include "silver_bullets/sync/CancelController.hpp"

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
struct TaskExecutorCancelParam<TaskFunc, std::enable_if_t<IsCancellable_v<TaskFunc>, void>>
{
    using type = sync::CancelController::Checker;
    static constexpr bool isCancelled(const type& c) {
        return c;
    }
};

template<class TaskFunc>
struct TaskExecutorCancelParam<TaskFunc, std::enable_if_t<!IsCancellable_v<TaskFunc>, void>>
{
    struct type {};
    static constexpr bool isCancelled(const type&) {
        return false;
    }
};

} // namespace task_engine
} // namespace silver_bullets
