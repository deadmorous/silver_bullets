#pragma once

#include <cstddef>

namespace silver_bullets {
namespace task_engine {

struct Task
{
    std::size_t inputCount = 0;
    std::size_t outputCount = 0;
    int taskFuncId = 0;
    int resourceType = 0;
};

} // namespace task_engine
} // namespace silver_bullets
