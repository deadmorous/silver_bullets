#pragma once

#include <cstddef>

namespace silver_bullets {
namespace task_engine {

struct InputEndPoint
{
    size_t taskId;
    size_t inputPort;
};

inline bool operator<(const InputEndPoint& a, const InputEndPoint& b) {
    return a.taskId == b.taskId? a.inputPort < b.inputPort: a.taskId < b.taskId;
}

} // namespace task_engine
} // namespace silver_bullets
