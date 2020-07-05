#pragma once

#include "types.hpp"
#include "Task.hpp"
#include "Connection.hpp"

#include <vector>
#include <map>

#include <boost/assert.hpp>

namespace silver_bullets {
namespace task_engine {

struct TaskGraph
{
    struct TaskInfo {
        Task task;

        // inputs of the task are determined by indices in dataMap range
        // [inputIndex, inputIndex+task.inputCount)
        std::size_t inputIndex;

        // outputs of the task are determined by indices in dataMap range
        // [outputIndex, outputIndex+task.outputCount)
        std::size_t outputIndex;
    };
    std::vector<TaskInfo> taskInfo;
    std::vector<Connection> connections;
    std::map<int, std::size_t> resourceCapacity;
    std::vector<boost::any> data;   // All data elements, including inputs and outputs
    std::vector<size_t> dataMap;    // Each element is an index in data

    boost::any& input(std::size_t taskId, std::size_t inputPort)
    {
        BOOST_ASSERT(taskId < taskInfo.size());
        auto& ti = taskInfo[taskId];
        BOOST_ASSERT(inputPort < ti.task.inputCount);
        auto dataIndex = dataMap[ti.inputIndex+inputPort];
        BOOST_ASSERT(dataIndex < data.size());
        return data[dataIndex];
    }

    const boost::any& output(std::size_t taskId, std::size_t outputPort) const
    {
        BOOST_ASSERT(taskId < taskInfo.size());
        auto& ti = taskInfo[taskId];
        BOOST_ASSERT(outputPort < ti.task.outputCount);
        auto dataIndex = dataMap[ti.outputIndex+outputPort];
        BOOST_ASSERT(dataIndex < data.size());
        return data[dataIndex];
    }
};

} // namespace task_engine
} // namespace silver_bullets
