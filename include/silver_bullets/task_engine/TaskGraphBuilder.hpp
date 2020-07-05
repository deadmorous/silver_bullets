#pragma once

#include "TaskGraph.hpp"

#include <boost/assert.hpp>

namespace silver_bullets {
namespace task_engine {

class TaskGraphBuilder
{
public:
    size_t addTask(
            std::size_t inputCount,
            std::size_t outputCount,
            int taskFuncId,
            int resourceType)
    {
        auto result = m_tasks.size();
        m_tasks.emplace_back(Task{inputCount, outputCount, taskFuncId, resourceType});
        return result;
    }

    void connect(
            std::size_t sourceTaskId,
            std::size_t sourcePort,
            std::size_t sinkTaskId,
            std::size_t sinkPort)
    {
        m_connections.emplace_back(
            Connection{{sourceTaskId, sourcePort}, {sinkTaskId, sinkPort}});
    }

    TaskGraph taskGraph() const
    {
        TaskGraph result;
        size_t dataMapSize = 0;
        for (auto& t : m_tasks)
            dataMapSize += t.inputCount + t.outputCount;
        BOOST_ASSERT(m_connections.size() < dataMapSize);
        auto dataSize = dataMapSize - m_connections.size();
        result.taskInfo.reserve(m_tasks.size());
        result.data.resize(dataSize);
        result.dataMap.resize(dataMapSize);
        result.connections = m_connections;

        std::map<InputEndPoint, OutputEndPoint> i2o;
        for (auto c : m_connections) {
            if (c.from.taskId >= m_tasks.size() ||
                    c.from.outputPort >= m_tasks[c.from.taskId].outputCount ||
                    c.to.taskId >= m_tasks.size() ||
                    c.to.inputPort >= m_tasks[c.to.taskId].inputCount)
                throw std::invalid_argument("TaskGraphBuilder: invalid connection");
            if (i2o.find(c.to) != i2o.end())
                throw std::invalid_argument("TaskGraphBuilder: multiple connetions to the same input port");
            i2o.insert({c.to, c.from});
        }

        size_t idata = 0;
        size_t imap = 0;
        for (size_t taskId=0, n=m_tasks.size(); taskId<n; ++taskId) {
            auto& task = m_tasks[taskId];
            auto outputIndex = imap;
            for (std::size_t outputPort=0; outputPort<task.outputCount; ++outputPort)
                result.dataMap[imap++] = idata++;
            result.taskInfo.emplace_back(TaskGraph::TaskInfo{task, 0, outputIndex});
        }
        for (size_t taskId=0, n=m_tasks.size(); taskId<n; ++taskId) {
            auto& task = m_tasks[taskId];
            result.taskInfo[taskId].inputIndex = imap;
            for (std::size_t inputPort=0; inputPort<task.inputCount; ++inputPort) {
                InputEndPoint input = { taskId, inputPort };
                auto it = i2o.find(input);
                if (it == i2o.end())
                    result.dataMap[imap++] = idata++;
                else {
                    auto output = it->second;
                    auto& adjTaskInfo = result.taskInfo[output.taskId];
                    result.dataMap[imap++] = result.dataMap[adjTaskInfo.outputIndex + output.outputPort];
                }
            }
        }
        return result;
    }

private:
    std::vector<Task> m_tasks;
    std::vector<Connection> m_connections;
};

} // namespace task_engine
} // namespace silver_bullets
