#pragma once

#include <cstddef>

namespace silver_bullets {
namespace task_engine {

struct OutputEndPoint
{
    size_t taskId;
    size_t outputPort;
};

inline bool operator<(const OutputEndPoint& a, const OutputEndPoint& b) {
    return a.taskId == b.taskId? a.outputPort < b.outputPort: a.taskId < b.taskId;
}

} // namespace task_engine
} // namespace silver_bullets
