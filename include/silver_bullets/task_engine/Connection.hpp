#pragma once

#include "InputEndPoint.hpp"
#include "OutputEndPoint.hpp"

namespace silver_bullets {
namespace task_engine {

struct Connection
{
    OutputEndPoint from;
    InputEndPoint to;
};

} // namespace task_engine
} // namespace silver_bullets
