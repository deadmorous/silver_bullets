#pragma once

#include <map>

namespace silver_bullets {
namespace task_engine {

template<class TaskFunc>
using TaskFuncRegistry = std::map<int, TaskFunc>;

} // namespace task_engine
} // namespace silver_bullets
