#pragma once

#include "TaskGraph.hpp"
#include "TaskExecutor.hpp"

#include <memory>
#include <thread>
#include <unordered_set>

#include <boost/range/algorithm/copy.hpp>
#include <boost/assert.hpp>

namespace silver_bullets {
namespace task_engine {

template <class TaskFunc>
class TaskGraphExecutor
{
public:
    using Cb = std::function<void()>;

    TaskGraphExecutor& addTaskExecutor(const std::shared_ptr<TaskExecutor<TaskFunc>>& taskExecutor)
    {
        m_resourceInfo[taskExecutor->resourceType()].executorInfo.push_back({taskExecutor});
        return *this;
    }

    boost::any makeCache() {
        return Cache();
    }

    TaskGraphExecutor& start(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry<TaskFunc> *taskFuncRegistry,
            const Cb& cb)
    {
        BOOST_ASSERT(!m_cb);
        m_cb = cb;
        startPriv(taskGraph, cache, taskFuncRegistry);
        return *this;
    }

    TaskGraphExecutor& start(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry<TaskFunc> *taskFuncRegistry)
    {
        BOOST_ASSERT(!m_cb);
        startPriv(taskGraph, cache, taskFuncRegistry);
        return *this;
    }

    TaskGraphExecutor& join()
    {
        while(m_running) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            propagateCb();
        }
        return *this;
    }

    bool propagateCb()
    {
        if (m_running) {
            // Track finished tasks
            BOOST_ASSERT(m_totalComputedOutputCount < m_cache->totalOutputCount);
            for (auto& resourceInfoItem : m_resourceInfo) {
                auto& ri = resourceInfoItem.second;
                for (std::size_t i=0; i<ri.runningExecutorCount; ++i) {
                    auto& xi = ri.executorInfo[i];
                    if (xi.executor->propagateCb()) {
                        // Executor has finished task

                        // Update the total number of computed outputs
                        auto& ti = m_taskGraph->taskInfo[xi.taskId];
                        m_totalComputedOutputCount += ti.task.outputCount;

                        // Update available input counters for connected tasks;
                        // enqueue next tasks, if any
                        for (std::size_t outputPort=0; outputPort<ti.task.outputCount; ++outputPort) {
                            auto itr = m_cache->o2i.equal_range({xi.taskId, outputPort});
                            for (auto& o2iItem : boost::make_iterator_range(itr.first, itr.second)) {
                                auto adjTaskId = o2iItem.second.taskId;
                                auto availAdjInputCount = ++m_cache->availTaskInputs[adjTaskId];
                                if (availAdjInputCount == m_taskGraph->taskInfo[adjTaskId].task.inputCount)
                                    m_ready.insert(adjTaskId);
                            }
                        }

                        // Put the executor after the last currently executing one
                        xi.taskId = ~0;
                        BOOST_ASSERT(ri.runningExecutorCount > 0);
                        --ri.runningExecutorCount;
                        if (i != ri.runningExecutorCount) {
                            BOOST_ASSERT(i < ri.runningExecutorCount);
                            std::swap(xi, ri.executorInfo[ri.runningExecutorCount]);
                            --i;
                        }
                    }
                }
            }

            // Start new tasks
            startNextTasks();
        }
        else
            return false;
        if (m_totalComputedOutputCount == m_cache->totalOutputCount) {
            m_running = false;
            if (m_cb)
                m_cb();
            m_cb = Cb();
            m_taskGraph = nullptr;
            m_cache = nullptr;
            m_taskFuncRegistry = nullptr;
        }
        return !m_running;
    }

private:
    struct ExecutorInfo {
        std::shared_ptr<TaskExecutor<TaskFunc>> executor;
        std::size_t taskId = ~0;
    };
    struct ResourceInfo
    {
        std::vector<ExecutorInfo> executorInfo;
        std::size_t runningExecutorCount = 0;   // Running are all at the beginning of executors
    };

    std::map<int, ResourceInfo> m_resourceInfo;

    struct Cache
    {
        std::vector<std::size_t> roots; // taskIds of tasks with all inputs initially available
        std::multimap<OutputEndPoint, InputEndPoint> o2i;
        std::size_t totalOutputCount = 0;
        // index=taskId, value=number of inputs initially available
        std::vector<std::size_t> initAvailTaskInputs;
        struct TaskIoDataIdx {
            std::size_t inputIndex = 0;     // starting index of task inputs in dataPtrs
            std::size_t outputIndex = 0;    // starting index of task outputs in dataPtrs
        };
        std::vector<TaskIoDataIdx> taskIoDataIdx;   // index = taskId

        // index=taskId, value=number of inputs currently available
        mutable std::vector<std::size_t> availTaskInputs;

        // Index is TaskGraph::TaskInfo::inputIndex + inputPort or
        // TaskGraph::TaskInfo::outputIndex + outputPort,
        // value = pointer to corresponding input/output value
        mutable std::vector<boost::any*> dataPtrs;
    };

    bool m_running = false;
    Cb m_cb;
    TaskGraph *m_taskGraph = nullptr;
    std::size_t m_totalComputedOutputCount = 0;
    const Cache *m_cache = nullptr;
    const TaskFuncRegistry<TaskFunc> *m_taskFuncRegistry = nullptr;

    // Elements are taskIds of tasks with all inputs available, whose processing has not started yet.
    std::unordered_set<std::size_t> m_ready;

    void startPriv(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry<TaskFunc> *taskFuncRegistry)
    {
        BOOST_ASSERT(!m_running);
        BOOST_ASSERT(!m_taskGraph);
        BOOST_ASSERT(!m_cache);
        BOOST_ASSERT(!m_taskFuncRegistry);
        m_running = true;
        m_taskGraph = taskGraph;
        m_totalComputedOutputCount = 0;
        auto mcache = &boost::any_cast<Cache&>(cache);
        m_cache = mcache;
        m_taskFuncRegistry = taskFuncRegistry;
        if (mcache->roots.empty()) {
            // Build cache

            auto taskCount = m_taskGraph->taskInfo.size();

            // Compute i2o and o2i maps
            BOOST_ASSERT(mcache->o2i.empty());
            std::map<InputEndPoint, OutputEndPoint> i2o;
            for (auto& c : m_taskGraph->connections) {
                i2o[c.to] = c.from;
                mcache->o2i.insert({c.from, c.to});
            }

            // Compute roots, initAvailTaskInputs, totalOutputCount,
            // taskIoDataIdx, and dataPtrs
            mcache->initAvailTaskInputs.resize(taskCount);
            mcache->taskIoDataIdx.resize(taskCount);
            mcache->dataPtrs.resize(m_taskGraph->dataMap.size());
            std::size_t idataPtrs = 0;
            BOOST_ASSERT(mcache->totalOutputCount == 0);
            for (std::size_t taskId=0; taskId<taskCount; ++taskId) {
                // Compute available inputs
                auto& ti = m_taskGraph->taskInfo[taskId];
                std::size_t availInputCount = 0;
                for (std::size_t inputPort=0; inputPort<ti.task.inputCount; ++inputPort)
                    if (i2o.find({taskId, inputPort}) == i2o.end())
                        ++availInputCount;
                mcache->initAvailTaskInputs[taskId] = availInputCount;

                // Make root if all inputs are available
                if (availInputCount == ti.task.inputCount)
                    mcache->roots.push_back(taskId);

                // Update total output count
                mcache->totalOutputCount += ti.task.outputCount;

                // Initialize taskIoDataIdx and dataPtrs
                mcache->taskIoDataIdx[taskId].inputIndex = idataPtrs;
                for (std::size_t inputPort=0; inputPort<ti.task.inputCount; ++inputPort)
                    mcache->dataPtrs[idataPtrs++] =
                            &m_taskGraph->data[m_taskGraph->dataMap[ti.inputIndex+inputPort]];
                mcache->taskIoDataIdx[taskId].outputIndex = idataPtrs;
                for (std::size_t outputPort=0; outputPort<ti.task.outputCount; ++outputPort)
                    mcache->dataPtrs[idataPtrs++] =
                            &m_taskGraph->data[m_taskGraph->dataMap[ti.outputIndex+outputPort]];
            }
            mcache->availTaskInputs = mcache->initAvailTaskInputs;
        }
        else
            // Initialize cache mutable data
            boost::range::copy(mcache->initAvailTaskInputs, mcache->availTaskInputs.begin());

        m_ready.clear();
        boost::range::copy(mcache->roots, std::inserter(m_ready, m_ready.end()));

        // Start all or part of root tasks
        startNextTasks();
    }

    bool startNextTasks()
    {
        // Start tasks
        std::vector<std::size_t> justStarted;
        for (auto& taskId : m_ready) {
            auto& ti = m_taskGraph->taskInfo[taskId];
            auto it = m_resourceInfo.find(ti.task.resourceType);
            if (it == m_resourceInfo.end())
                throw std::runtime_error("TaskGraphExecutor: No suitable resources are supplied");
            auto& ri = it->second;
            if (ri.runningExecutorCount < ri.executorInfo.size()) {
                // Start new task
                auto& xi = ri.executorInfo[ri.runningExecutorCount];
                xi.taskId = taskId;
                auto d = m_cache->dataPtrs.data();
                auto outputIndex = m_cache->taskIoDataIdx[taskId].outputIndex;
                auto inputIndex = m_cache->taskIoDataIdx[taskId].inputIndex;
                xi.executor->start(
                            ti.task,
                            { d+outputIndex, d+outputIndex+ti.task.outputCount },
                            { d+inputIndex, d+inputIndex+ti.task.inputCount },
                            [](){},
                            *m_taskFuncRegistry);
                ++ri.runningExecutorCount;
                justStarted.push_back(taskId);
            }
        }

        // Remove started tasks from m_ready
        for (auto taskId : justStarted)
            m_ready.erase(taskId);

        return !justStarted.empty();
    }
};

} // namespace task_engine
} // namespace silver_bullets
