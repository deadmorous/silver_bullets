#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_set>

#include <boost/assert.hpp>
#include <boost/any.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/int.hpp>

namespace silver_bullets {

using const_pany_range = boost::iterator_range<boost::any const* const*>;
using pany_range = boost::iterator_range<boost::any**>;

using TaskFunc = std::function<void(const pany_range&, const const_pany_range&)>;



using TaskFuncRegistry = std::map<int, TaskFunc>;

struct Task
{
    std::size_t inputCount = 0;
    std::size_t outputCount = 0;
    int taskFuncId = 0;
    int resourceType = 0;
};



struct InputEndPoint
{
    size_t taskId;
    size_t inputPort;
};

struct less_input_pt {
    bool operator()(const InputEndPoint& a, const InputEndPoint& b) const {
        return a.taskId == b.taskId? a.inputPort < b.inputPort: a.taskId < b.taskId;
    }
};

struct OutputEndPoint
{
    size_t taskId;
    size_t outputPort;
};

struct less_output_pt {
    bool operator()(const OutputEndPoint& a, const OutputEndPoint& b) const {
        return a.taskId == b.taskId? a.outputPort < b.outputPort: a.taskId < b.taskId;
    }
};



struct Connection
{
    OutputEndPoint from;
    InputEndPoint to;
};

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
            const TaskFuncRegistry& taskFuncRegistry) = 0;
    virtual bool propagateCb() = 0;
};

class ThreadedTaskExecutor : public TaskExecutor
{
public:
    explicit ThreadedTaskExecutor(int resourceType) :
        m_resourceType(resourceType),
        m_thread([this]() { run(); })
    {}

    ~ThreadedTaskExecutor()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_flags |= ExitRequested;
        lk.unlock();
        m_cond.notify_one();
        m_thread.join();
    }

    int resourceType() const override {
        return m_resourceType;
    }

    void start(
            const Task& task,
            const pany_range& outputs,
            const const_pany_range& inputs,
            const Cb& cb,
            const TaskFuncRegistry& taskFuncRegistry) override
    {
        BOOST_ASSERT(!m_f);
        BOOST_ASSERT(!m_cb);
        m_f = taskFuncRegistry.at(task.taskFuncId);
        m_outputs = outputs;
        m_inputs = inputs;
        m_cb = cb;
        std::unique_lock<std::mutex> lk(m_mutex);
        m_flags = HasInput;
        lk.unlock();
        m_cond.notify_one();
    }

    bool propagateCb() override
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        if (m_flags & HasOutput) {
            m_flags = 0;
            m_cb();
            m_f = TaskFunc();
            m_cb = Cb();
            return true;
        }
        else
            return false;
    }

private:
    int m_resourceType;
    TaskFunc m_f;
    pany_range m_outputs;
    const_pany_range m_inputs;
    Cb m_cb;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::thread m_thread;
    enum {
        HasInput = 0x01,
        HasOutput = 0x2,
        ExitRequested = 0x4
    };
    unsigned int m_flags = 0;

    void run() {
        while (true) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond.wait(lk, [this] {
                return !!(m_flags & (HasInput | ExitRequested));
            });
            if (m_flags & ExitRequested)
                return;
            m_f(m_outputs, m_inputs);
            m_flags = HasOutput;
        }
    }
};

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

//template<class F> inline TaskFunc taskFunc_2(F f)
//{
//    return [f](pany_range out, const_pany_range in) {
//        BOOST_ASSERT(out.size() == 1);
//        BOOST_ASSERT(in.size() == 2);
//        using args_t = typename boost::function_types::parameter_types<decltype(&F::operator())>::type;
//        using A1 = typename boost::mpl::at<args_t, boost::mpl::int_<1>>::type;
//        using A2 = typename boost::mpl::at<args_t, boost::mpl::int_<2>>::type;
//        *(out[0]) = f(boost::any_cast<A1>(*(in[0])), boost::any_cast<A2>(*(in[1])));
//    };
//}

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

        std::map<InputEndPoint, OutputEndPoint, less_input_pt> i2o;
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

class TaskGraphExecutor
{
public:
    using Cb = std::function<void()>;

    void addTaskExecutor(const std::shared_ptr<TaskExecutor>& taskExecutor) {
        m_resourceInfo[taskExecutor->resourceType()].executorInfo.push_back({taskExecutor});
    }

    boost::any makeCache() {
        return Cache();
    }

    void start(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry *taskFuncRegistry,
            const Cb& cb)
    {
        BOOST_ASSERT(!m_cb);
        m_cb = cb;
        startPriv(taskGraph, cache, taskFuncRegistry);
    }

    void start(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry *taskFuncRegistry)
    {
        BOOST_ASSERT(!m_cb);
        startPriv(taskGraph, cache, taskFuncRegistry);
    }

    void join()
    {
        while(m_running) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            propagateCb();
        }
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
        std::shared_ptr<TaskExecutor> executor;
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
        std::multimap<OutputEndPoint, InputEndPoint, less_output_pt> o2i;
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
    const TaskFuncRegistry *m_taskFuncRegistry = nullptr;

    // Elements are taskIds of tasks with all inputs available, whose processing has not started yet.
    std::unordered_set<std::size_t> m_ready;

    void startPriv(
            TaskGraph *taskGraph,
            boost::any& cache,
            const TaskFuncRegistry *taskFuncRegistry)
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
            std::map<InputEndPoint, OutputEndPoint, less_input_pt> i2o;
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

} // namespace silver_bullets
